/*
 *  This file is part of RawTherapee.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ttseriesexposure.h"
#include "ttlog.h"
#include "esbyoptions.h"
#include "multilangmgr.h"
#include "variable.h"
#include "esbysharedvars.h"
#include "tools/tonecurve.h"
#include "rtengine/color.h"
#include "rtengine/procparams.h"
#include <cjson/cJSON.h>
#include <exiv2/exiv2.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace rtengine;
using namespace rtengine::procparams;

// limits of the exposure compensation adjuster (tools/tonecurve.cc)
static const double SERIESEXP_MIN_COMP = -5.0;
static const double SERIESEXP_MAX_COMP = 12.0;
// two compensations closer than this are the same
static const double SERIESEXP_EPS = 0.005;
// values of the follow combo, in its order
static const char* const FOLLOW_VALUES[] = {"nearest", "series", "parent", "none"};
static const char REFERENCE_VAR[] = "exposure.reference";
static const char OFFSET_VAR[] = "exposure.offset";

static Glib::ustring ev(double value)
{
  return Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), value);
}

static double clampComp(double value)
{
  return std::max(SERIESEXP_MIN_COMP, std::min(SERIESEXP_MAX_COMP, value));
}

// ---------------------------------------------------------------------------------------
// reference (JSON text of the exposure.reference variable)
// ---------------------------------------------------------------------------------------

TTSeriesExposure::Reference TTSeriesExposure::Reference::parse(const Glib::ustring& text, const Glib::ustring& origin)
{
  Reference r;
  cJSON* json = cJSON_Parse(text.c_str());
  if (json == nullptr)
    return r;
  cJSON* raw = cJSON_GetObjectItem(json, "raw");
  cJSON* comp = cJSON_GetObjectItem(json, "comp");
  cJSON* point = cJSON_GetObjectItem(json, "point");
  cJSON* target = cJSON_GetObjectItem(json, "target");
  if (cJSON_IsString(raw) && cJSON_IsNumber(comp))
  {
    r.valid = true;
    r.raw = raw->valuestring;
    r.comp = comp->valuedouble;
    r.origin = origin;
    if (cJSON_IsArray(point) && (cJSON_GetArraySize(point) == 2))
    {
      r.hasPoint = true;
      r.px = cJSON_GetArrayItem(point, 0)->valuedouble;
      r.py = cJSON_GetArrayItem(point, 1)->valuedouble;
    }
    if (cJSON_IsNumber(target) && (target->valuedouble > 0.0))
    {
      r.hasTarget = true;
      r.target = target->valuedouble;
    }
  }
  cJSON_Delete(json);
  return r;
}

Glib::ustring TTSeriesExposure::Reference::serialize() const
{
  cJSON* json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "raw", raw.c_str());
  cJSON_AddNumberToObject(json, "comp", comp);
  if (hasPoint)
  {
    cJSON* p = cJSON_AddArrayToObject(json, "point");
    cJSON_AddItemToArray(p, cJSON_CreateNumber(px));
    cJSON_AddItemToArray(p, cJSON_CreateNumber(py));
  }
  if (hasTarget)
    cJSON_AddNumberToObject(json, "target", target);
  char* text = cJSON_PrintUnformatted(json);
  Glib::ustring result(text);
  cJSON_free(text);
  cJSON_Delete(json);
  return result;
}

// ---------------------------------------------------------------------------------------
// interface
// ---------------------------------------------------------------------------------------

TTSeriesExposure::TTSeriesExposure() :
  FoldableToolPanel(this, "TTSeriesExposure", M("TT_SERIESEXP_LABEL"), false, true),
  EditSubscriber(ET_PIPETTE)
{
  tonecurve = nullptr;
  sequenceReady = false;
  stateLoaded = false;
  follow = "nearest";
  hasDelta = hasLastComp = hasShownComp = false;
  delta = lastComp = shownComp = 0.0;
  refsPending = 0;
  generation = 0;
  refGeneration = 0;
  hasSuggestion = false;
  suggestedDelta = 0.0;
  recomputePending = false;
  sharedConnected = false;
  focusConnected = false;
  updatingWidgets = false;

  // the pipette of the first tone curve of the exposure tool: values after the exposure
  // compensation, the brightness, the contrast and the highlight compression, before the curves
  setEditID(EUID_ToneCurve1, BT_IMAGEFLOAT);

  Gtk::FlowBox* refBox = Gtk::manage(new Gtk::FlowBox());
  refBox->set_selection_mode(Gtk::SELECTION_NONE);
  refBox->set_column_spacing(4);
  refBox->set_row_spacing(2);
  tbPipette = Gtk::manage(new Gtk::ToggleButton(M("TT_SERIESEXP_PIPETTE")));
  tbPipette->set_tooltip_text(M("TT_SERIESEXP_PIPETTE_TOOLTIP"));
  cbLevel = Gtk::manage(new MyComboBoxText());
  cbLevel->append(M("TT_SERIESEXP_LEVEL_SERIES"));
  cbLevel->append(M("TT_SERIESEXP_LEVEL_PARENT"));
  cbLevel->set_active(0);
  cbLevel->set_tooltip_text(M("TT_SERIESEXP_LEVEL_TOOLTIP"));
  btReference = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_REFERENCE")));
  btReference->set_tooltip_text(M("TT_SERIESEXP_REFERENCE_TOOLTIP"));
  btNoReference = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_NO_REFERENCE_BUTTON")));
  btNoReference->set_tooltip_text(M("TT_SERIESEXP_NO_REFERENCE_TOOLTIP"));
  refBox->add(*tbPipette);
  refBox->add(*cbLevel);
  refBox->add(*btReference);
  refBox->add(*btNoReference);
  pack_start(*refBox, Gtk::PACK_SHRINK, 0);

  Gtk::FlowBox* imageBox = Gtk::manage(new Gtk::FlowBox());
  imageBox->set_selection_mode(Gtk::SELECTION_NONE);
  imageBox->set_column_spacing(4);
  imageBox->set_row_spacing(2);
  cbFollow = Gtk::manage(new MyComboBoxText());
  cbFollow->append(M("TT_SERIESEXP_FOLLOW_NEAREST"));
  cbFollow->append(M("TT_SERIESEXP_FOLLOW_SERIES"));
  cbFollow->append(M("TT_SERIESEXP_FOLLOW_PARENT"));
  cbFollow->append(M("TT_SERIESEXP_FOLLOW_NONE"));
  cbFollow->set_active(0);
  cbFollow->set_tooltip_text(M("TT_SERIESEXP_FOLLOW_TOOLTIP"));
  btMeasure = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_MEASURE")));
  btMeasure->set_tooltip_text(M("TT_SERIESEXP_MEASURE_TOOLTIP"));
  btApply = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_APPLY")));
  btApply->set_tooltip_text(M("TT_SERIESEXP_APPLY_TOOLTIP"));
  btApply->set_sensitive(false);
  imageBox->add(*cbFollow);
  imageBox->add(*btMeasure);
  imageBox->add(*btApply);
  pack_start(*imageBox, Gtk::PACK_SHRINK, 0);

  // offset of the series: set on the series of the image (esby server), seen by every instance
  adjOffset = Gtk::manage(new Adjuster(M("TT_SERIESEXP_OFFSET"), -2.0, 2.0, 0.05, 0.0));
  adjOffset->setAdjusterListener(this);
  adjOffset->set_tooltip_text(M("TT_SERIESEXP_OFFSET_TOOLTIP"));
  adjOffset->set_sensitive(false);
  pack_start(*adjOffset, Gtk::PACK_SHRINK, 0);

  lbInfo = Gtk::manage(new Gtk::Label(""));
  lbInfo->set_line_wrap(true);
  lbInfo->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
  lbInfo->set_xalign(0.0);
  pack_start(*lbInfo, Gtk::PACK_SHRINK, 0);

  service.reset(new EsbyHttpClient());
  service->setAddress(esbySettings().SeriesExpService);
  const char* token = g_getenv("ESBY_EXPOSURE_TOKEN");
  if (token != nullptr)
    service->setToken(token);
  btReference->set_sensitive(service->isEnabled());
  btMeasure->set_sensitive(service->isEnabled());

  btReference->signal_clicked().connect([this]() { setReferenceHere(false, 0.0, 0.0, false, 0.0); });
  btNoReference->signal_clicked().connect([this]() { removeReference(); });
  btMeasure->signal_clicked().connect([this]() { measureCurrent(true); });
  btApply->signal_clicked().connect([this]() {
    if (!hasSuggestion)
      return;
    hasSuggestion = false;
    btApply->set_sensitive(false);
    storeDelta(suggestedDelta);
    recompute(false);
  });
  cbFollow->signal_changed().connect([this]() {
    if (updatingWidgets)
      return;
    int row = cbFollow->get_active_row_number();
    if ((row >= 0) && (row < 4))
      setFollow(FOLLOW_VALUES[row], true);
  });
  tbPipette->signal_toggled().connect([this]() {
    if (tbPipette->get_active())
    {
      if (getEditProvider() != nullptr)
        subscribe();
    }
    else if (isCurrentSubscriber())
      unsubscribe();
  });
}

TTSeriesExposure::~TTSeriesExposure()
{
}

void TTSeriesExposure::deploy()
{
  FoldableToolPanel::deploy();

  for (size_t i = 0; i < env->getToolPanels().size(); i++)
  {
    ToolPanel* p = static_cast<ToolPanel*>(env->getPanel(i));
    if ((p != nullptr) && (p->getToolName() == "tonecurve"))
      tonecurve = static_cast<ToneCurve*>(p);
  }
}

void TTSeriesExposure::deployLate()
{
}

void TTSeriesExposure::setEditProvider(EditDataProvider* provider)
{
  EditSubscriber::setEditProvider(provider);
}

void TTSeriesExposure::switchOffEditMode()
{
  EditSubscriber::switchOffEditMode();
  tbPipette->set_active(false);
}

// ---------------------------------------------------------------------------------------
// series
// ---------------------------------------------------------------------------------------

// ESBY_ORIGIN (see TTSeriesWB): the original file of a copy opened by a launcher such as rt_queue
Glib::ustring TTSeriesExposure::originalFile()
{
  return EsbySharedVariables::originalFile(env);
}

Glib::ustring TTSeriesExposure::sequenceFolder(const Glib::ustring& file)
{
  return EsbySharedVariables::sequenceFolder(file);
}

// the series of an image is its numbered folder
void TTSeriesExposure::startSequence()
{
  sequenceReady = false;
  Glib::ustring file = originalFile();
  if (file.empty())
  {
    setInfo(M("TT_SERIESEXP_NO_IMAGE"));
    return;
  }
  sequenceKey = EsbySharedVariables::realPath(sequenceFolder(file));
  sequenceText = Glib::ustring::compose(M("TT_SERIESEXP_SEQ"), Glib::path_get_basename(sequenceKey));
  publishSequence();
  loadImageState();
}

// the series is known: given to the shared variables (the offset is set on it)
void TTSeriesExposure::publishSequence()
{
  sequenceReady = true;
  if (env != nullptr)
    env->sharedVariables()->setSequence(originalFile(), Glib::ustring());
}

// relative luminance (linear) under the pipette
bool TTSeriesExposure::measure(double& luminance)
{
  EditDataProvider* provider = getEditProvider();
  if (provider == nullptr)
    return false;
  // the pipette values are encoded with the sRGB like curve of RawTherapee (Color::gamma2curve)
  double r = Color::igamma2(std::max(0.f, provider->getPipetteVal1()));
  double g = Color::igamma2(std::max(0.f, provider->getPipetteVal2()));
  double b = Color::igamma2(std::max(0.f, provider->getPipetteVal3()));
  luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;
  return luminance > 0.0;
}

double TTSeriesExposure::currentExpComp()
{
  ProcParams pp;
  tonecurve->write(&pp);
  return pp.toneCurve.expcomp;
}

void TTSeriesExposure::setExpComp(double value)
{
  ProcParams pp;
  tonecurve->write(&pp);
  pp.toneCurve.expcomp = value;
  tonecurve->read(&pp);
  if (listener)
    listener->panelChanged(EvExpComp, Glib::ustring::format(std::setw(5), std::fixed, std::setprecision(2), value));
}

// offset of the series: shared variable exposure.offset (0 when not set, or without server)
double TTSeriesExposure::seriesOffset()
{
  if (env == nullptr)
    return 0.0;
  RtVariable* v = env->getVariableByName(OFFSET_VAR);
  if ((v == nullptr) || !v->isDefined() || (v->getScope() == RtVariableScope::Internal) || (v->getScope() == RtVariableScope::Exif))
    return 0.0;
  return v->getAsDouble();
}

// automatic exposure or histogram matching: each image gets its own exposure, the compensation
// of the series does not hold anymore
bool TTSeriesExposure::exposureIsAutomatic()
{
  ProcParams pp;
  tonecurve->write(&pp);
  return pp.toneCurve.autoexp || pp.toneCurve.histmatching;
}

// ---------------------------------------------------------------------------------------
// state of the image and references (esby server)
// ---------------------------------------------------------------------------------------

void TTSeriesExposure::loadImageState()
{
  stateLoaded = false;
  stateFile = originalFile();
  follow = "nearest";
  hasDelta = hasLastComp = hasShownComp = false;
  hasSuggestion = false;
  btApply->set_sensitive(false);
  recomputePending = false;
  int current = ++generation;
  EsbyWBClient* client = env->sharedVariables()->getClient();
  if (!client->isConnected())
  {
    setInfo(M("TT_SERIESEXP_NO_SERVER"));
    return;
  }
  client->fileGet(stateFile, "exposure", [this, current](bool ok, const std::vector<EsbyVarValue>& fields) {
    if (current != generation)
      return;
    for (const EsbyVarValue& f : fields)
    {
      double number = (f.type == RT_VARIABLE_TYPE_INT) ? f.i : f.d;
      if (f.name == "follow")
        follow = f.s;
      else if (f.name == "delta")
      {
        hasDelta = true;
        delta = number;
      }
      else if (f.name == "comp")
      {
        hasLastComp = true;
        lastComp = number;
      }
    }
    stateLoaded = ok;
    updatingWidgets = true;
    for (int i = 0; i < 4; i++)
      if (follow == FOLLOW_VALUES[i])
        cbFollow->set_active(i);
    updatingWidgets = false;
    loadReferences();
  });
}

// the reference of the series (set on its folder) and the one of the parent (nearest above it)
void TTSeriesExposure::loadReferences()
{
  EsbyWBClient* client = env->sharedVariables()->getClient();
  if (sequenceKey.empty() || !client->isConnected())
    return;
  // a new load replaces one still running
  int current = ++refGeneration;
  refsPending = 2;
  auto done = [this, current]() {
    if ((current == refGeneration) && (--refsPending == 0))
      recompute(true);
  };
  client->varGet(sequenceKey, "", [this, current, done](bool, const std::vector<EsbyVarValue>& vars) {
    if (current != refGeneration)
      return;
    refSeries = Reference();
    for (const EsbyVarValue& v : vars)
      if ((v.name == REFERENCE_VAR) && (v.origin == sequenceKey))
        refSeries = Reference::parse(v.s, v.origin);
    done();
  });
  client->varGet(Glib::path_get_dirname(sequenceKey), "", [this, current, done](bool, const std::vector<EsbyVarValue>& vars) {
    if (current != refGeneration)
      return;
    refParent = Reference();
    for (const EsbyVarValue& v : vars)
      if (v.name == REFERENCE_VAR)
        refParent = Reference::parse(v.s, v.origin);
    done();
  });
}

const TTSeriesExposure::Reference& TTSeriesExposure::followedReference()
{
  if (follow == "series")
    return refSeries;
  if (follow == "parent")
    return refParent;
  return refSeries.valid ? refSeries : refParent; // nearest
}

// session of the measuring service: one per reference (where it is set, and its raw file)
Glib::ustring TTSeriesExposure::sessionOf(const Reference& ref)
{
  return ref.origin + "|" + Glib::path_get_basename(ref.raw);
}

// the compensation of the image follows its reference: reference + delta + offset.
// fromEvent: a change of a reference, of the offset or of the state; it is applied in the active
// window only (N instances do not recalculate together), the others do it when they get the focus
void TTSeriesExposure::recompute(bool fromEvent)
{
  if ((env == nullptr) || (tonecurve == nullptr) || !getExpander()->getEnabled() || !sequenceReady
  || !stateLoaded || (refsPending > 0) || (stateFile != originalFile()))
    return;
  if (follow == "none")
  {
    setInfo(M("TT_SERIESEXP_NOT_APPLIED"));
    return;
  }
  const Reference& ref = followedReference();
  if (!ref.valid)
  {
    setInfo(M("TT_SERIESEXP_NO_REFERENCE"));
    return;
  }
  if (exposureIsAutomatic())
  {
    setInfo(M("TT_SERIESEXP_AUTOMATIC"));
    return;
  }
  double current = currentExpComp();
  // changed by hand since the tool set it (in this instance, or in any instance for an image just
  // opened): the image stops following its reference
  bool known = hasShownComp || hasLastComp;
  double expected = hasShownComp ? shownComp : lastComp;
  if (known && (std::fabs(current - expected) >= SERIESEXP_EPS))
  {
    setFollow("none", true);
    setInfo(Glib::ustring::compose(M("TT_SERIESEXP_CHANGED_BY_HAND"), ev(current), ev(expected)));
    return;
  }
  if (!hasDelta)
  {
    if (Glib::path_get_basename(originalFile()) == Glib::path_get_basename(ref.raw))
      storeDelta(0.0); // the reference itself
    else
    {
      if (service->isEnabled())
        measureCurrent(false);
      else
        setInfo(M("TT_SERIESEXP_NOT_MEASURED"));
      return;
    }
  }
  double comp = clampComp(ref.comp + delta + seriesOffset());
  if (std::fabs(current - comp) < SERIESEXP_EPS)
  {
    hasShownComp = true;
    shownComp = current;
    if (!hasLastComp || (std::fabs(lastComp - comp) >= SERIESEXP_EPS))
      saveApplied(comp);
    setInfo(Glib::ustring::compose(M("TT_SERIESEXP_FOLLOWING"), ev(ref.comp), ev(delta), ev(seriesOffset()), ev(comp)));
    return;
  }

  Gtk::Window* window = dynamic_cast<Gtk::Window*>(get_toplevel());
  if ((window != nullptr) && !focusConnected)
  {
    // the window becomes active in the default handler of focus-in: the change is applied just after
    window->signal_focus_in_event().connect([this](GdkEventFocus*) {
      if (recomputePending)
        Glib::signal_idle().connect_once([this]() { if (recomputePending) recompute(false); });
      return false;
    }, true);
    focusConnected = true;
  }
  if (fromEvent && (window != nullptr) && !window->is_active())
  {
    recomputePending = true;
    setInfo(Glib::ustring::compose(M("TT_SERIESEXP_PENDING"), ev(comp)));
    return;
  }
  recomputePending = false;
  setExpComp(comp);
  hasShownComp = true;
  shownComp = comp;
  saveApplied(comp);
  setInfo(Glib::ustring::compose(M("TT_SERIESEXP_FOLLOWING"), ev(ref.comp), ev(delta), ev(seriesOffset()), ev(comp)));
}

void TTSeriesExposure::setFollow(const Glib::ustring& f, bool save)
{
  follow = f;
  updatingWidgets = true;
  for (int i = 0; i < 4; i++)
    if (follow == FOLLOW_VALUES[i])
      cbFollow->set_active(i);
  updatingWidgets = false;
  if (save && !stateFile.empty())
  {
    EsbyVarValue v;
    v.name = "follow";
    v.s = follow;
    env->sharedVariables()->getClient()->fileSet(stateFile, "exposure", {v}, nullptr);
  }
  if (follow != "none")
  {
    // following again: the current compensation is the starting point
    hasShownComp = true;
    shownComp = currentExpComp();
    recompute(false);
  }
  else
    setInfo(M("TT_SERIESEXP_NOT_APPLIED"));
}

// the gap of the image with its reference: following it again if it was not applied
void TTSeriesExposure::storeDelta(double d)
{
  hasDelta = true;
  delta = d;
  EsbyVarValue v;
  v.name = "delta";
  v.type = RT_VARIABLE_TYPE_DOUBLE;
  v.d = d;
  std::vector<EsbyVarValue> fields = {v};
  if (follow == "none")
  {
    follow = "nearest";
    EsbyVarValue f;
    f.name = "follow";
    f.s = follow;
    fields.push_back(f);
    updatingWidgets = true;
    cbFollow->set_active(0);
    updatingWidgets = false;
  }
  hasShownComp = true;
  shownComp = currentExpComp();
  env->sharedVariables()->getClient()->fileSet(stateFile, "exposure", fields, nullptr);
}

void TTSeriesExposure::saveApplied(double comp)
{
  hasLastComp = true;
  lastComp = comp;
  EsbyVarValue v;
  v.name = "comp";
  v.type = RT_VARIABLE_TYPE_DOUBLE;
  v.d = comp;
  env->sharedVariables()->getClient()->fileSet(stateFile, "exposure", {v}, nullptr);
}

// the shared variables were reloaded (offset or reference changed, here or in another instance)
void TTSeriesExposure::sharedChanged()
{
  if (env == nullptr)
    return;
  EsbySharedVariables* shared = env->sharedVariables();
  adjOffset->set_sensitive(shared->isConnected() && sequenceReady);
  adjOffset->block(true);
  adjOffset->setValue(seriesOffset());
  adjOffset->block(false);
  if (!shared->isLoaded() || !getExpander()->getEnabled() || !sequenceReady)
    return;
  if (!stateLoaded && (stateFile == originalFile()))
  {
    loadImageState(); // the server was not available when the image was opened
    return;
  }
  loadReferences(); // then recompute
}

// the slider was moved: the offset is set on the series; the server sends the change to every
// instance (this one included), sharedChanged() applies it
void TTSeriesExposure::offsetAdjusted()
{
  if (!sequenceReady || (env == nullptr))
    return;
  EsbyVarValue value;
  value.name = OFFSET_VAR;
  value.type = RT_VARIABLE_TYPE_DOUBLE;
  value.d = adjOffset->getValue();
  env->sharedVariables()->set(EsbySharedVariables::Where::Sequence, value);
}

// ---------------------------------------------------------------------------------------
// references
// ---------------------------------------------------------------------------------------

// the current image becomes the reference of its series or of the parent folder (combo).
// Its compensation and its target are kept without the offset of the series.
void TTSeriesExposure::setReferenceHere(bool hasPoint, double px, double py, bool hasTarget, double target)
{
  if (!sequenceReady || (tonecurve == nullptr) || !env->sharedVariables()->isConnected())
  {
    setInfo(M(sequenceReady ? "TT_SERIESEXP_NO_SERVER" : "TT_SERIESEXP_NO_IMAGE"));
    return;
  }
  Reference ref;
  ref.valid = true;
  ref.raw = originalFile();
  ref.comp = currentExpComp() - seriesOffset();
  ref.hasPoint = hasPoint;
  ref.px = px;
  ref.py = py;
  ref.hasTarget = hasTarget;
  ref.target = target;
  Glib::ustring folder = (cbLevel->get_active_row_number() == 1) ? Glib::ustring(Glib::path_get_dirname(sequenceKey)) : sequenceKey;
  ref.origin = folder;
  if (!service->isEnabled())
  {
    if (!hasTarget)
    {
      setInfo(M("TT_SERIESEXP_NO_TARGET"));
      return;
    }
    publishReference(ref, folder);
    return;
  }

  // with the service: its face (at the point, or near the AF point, or the largest)
  cJSON* body = cJSON_CreateObject();
  cJSON_AddStringToObject(body, "session", sessionOf(ref).c_str());
  cJSON_AddStringToObject(body, "raw", ref.raw.c_str());
  if (hasPoint)
  {
    cJSON* p = cJSON_AddArrayToObject(body, "point");
    cJSON_AddItemToArray(p, cJSON_CreateNumber(px));
    cJSON_AddItemToArray(p, cJSON_CreateNumber(py));
  }
  double ax, ay;
  if (readAfPoint(env->getVarAsString("Fname"), ax, ay))
  {
    cJSON* p = cJSON_AddArrayToObject(body, "af_point");
    cJSON_AddItemToArray(p, cJSON_CreateNumber(ax));
    cJSON_AddItemToArray(p, cJSON_CreateNumber(ay));
  }
  setInfo(M("TT_SERIESEXP_WAITING"));
  Glib::ustring file = originalFile();
  service->post("/reference", body, [this, ref, file, folder](cJSON* answer, const Glib::ustring& error) {
    if (originalFile() != file)
      return;
    cJSON* status = answer ? cJSON_GetObjectItem(answer, "status") : nullptr;
    if (!cJSON_IsString(status))
    {
      setInfo(Glib::ustring::compose(M("TT_SERIESEXP_SERVICE_ERROR"), error));
      return;
    }
    if (Glib::ustring(status->valuestring) != "ok")
    {
      setInfo(Glib::ustring::compose(M("TT_SERIESEXP_STATUS"), status->valuestring));
      return;
    }
    publishReference(ref, folder);
  });
}

// the reference is set on the server (every instance follows it); the image is its own reference
void TTSeriesExposure::publishReference(const Reference& ref, const Glib::ustring& folder)
{
  EsbyVarValue value;
  value.name = REFERENCE_VAR;
  value.s = ref.serialize();
  env->sharedVariables()->getClient()->varSet(folder, "", value, nullptr);
  if (folder == sequenceKey)
    refSeries = ref;
  else
    refParent = ref;
  storeDelta(0.0);
  saveApplied(currentExpComp());
  // a reference of the parent set from a series having its own: this image follows the parent one
  if ((folder != sequenceKey) && refSeries.valid && (follow == "nearest"))
    setFollow("parent", true);
  setInfo(Glib::ustring::compose(M("TT_SERIESEXP_REFERENCE_SET"), Glib::path_get_basename(folder), ev(ref.comp)));
}

// the current image is not the reference of its series or of the parent anymore
void TTSeriesExposure::removeReference()
{
  Glib::ustring name = Glib::path_get_basename(originalFile());
  EsbyWBClient* client = env->sharedVariables()->getClient();
  bool removed = false;
  for (const Reference* ref : {&refSeries, &refParent})
  {
    if (ref->valid && (Glib::path_get_basename(ref->raw) == name))
    {
      client->varUnset(ref->origin, "", REFERENCE_VAR, nullptr);
      removed = true;
    }
  }
  setInfo(M(removed ? "TT_SERIESEXP_REFERENCE_REMOVED" : "TT_SERIESEXP_NOT_A_REFERENCE"));
}

// ---------------------------------------------------------------------------------------
// pipette
// ---------------------------------------------------------------------------------------

bool TTSeriesExposure::button1Pressed(int modifierKey)
{
  if (!getExpander()->getEnabled() || (tonecurve == nullptr))
    return false;
  if (!sequenceReady || !stateLoaded)
  {
    setInfo(M("TT_SERIESEXP_NO_SERVER"));
    return true;
  }
  double luminance;
  if (!measure(luminance))
  {
    setInfo(M("TT_SERIESEXP_NO_VALUE"));
    return true;
  }

  // ctrl + click: this image becomes the reference (combo: series or parent). The brightness is kept
  // as the target, without the offset; with the service, the point designates the face.
  if (modifierKey & GDK_CONTROL_MASK)
  {
    bool hasPoint = false;
    double px = 0.0, py = 0.0;
    if (service->isEnabled())
    {
      EditDataProvider* provider = getEditProvider();
      int w = 0, h = 0;
      provider->getImageSize(w, h);
      if ((w > 0) && (h > 0))
      {
        hasPoint = true;
        px = (double) provider->posImage.x / w;
        py = (double) provider->posImage.y / h;
      }
    }
    setReferenceHere(hasPoint, px, py, true, luminance / std::pow(2.0, seriesOffset()));
    return true;
  }

  // click: the gap with the target of the reference, measured on this image
  const Reference& ref = followedReference();
  if (!ref.valid || !ref.hasTarget)
  {
    setInfo(M("TT_SERIESEXP_NO_TARGET"));
    return true;
  }
  // linear values: the gap in EV is log2 of their ratio. The tone curve of the exposure tool
  // (contrast, highlight compression) makes it approximate: a second click refines it.
  double gap = std::log2(ref.target * std::pow(2.0, seriesOffset()) / luminance);
  double comp = clampComp(currentExpComp() + gap);
  storeDelta(comp - seriesOffset() - ref.comp);
  setExpComp(comp);
  hasShownComp = true;
  shownComp = comp;
  saveApplied(comp);
  setInfo(Glib::ustring::compose(M("TT_SERIESEXP_FOLLOWING"), ev(ref.comp), ev(delta), ev(seriesOffset()), ev(comp)));
  return true;
}

// ---------------------------------------------------------------------------------------
// external measuring service
// ---------------------------------------------------------------------------------------

void TTSeriesExposure::setSuggestion(double d, const Glib::ustring& text)
{
  suggestedDelta = d;
  hasSuggestion = true;
  btApply->set_sensitive(true);
  setInfo(text);
}

// measure of the current image by the service, against the face of its reference.
// manual: from the measure button (the result is then only proposed)
void TTSeriesExposure::measureCurrent(bool manual, bool retried)
{
  if (!service->isEnabled() || !sequenceReady || (tonecurve == nullptr) || !stateLoaded)
    return;
  Reference ref = followedReference();
  if (!ref.valid)
  {
    setInfo(M("TT_SERIESEXP_NO_REFERENCE"));
    return;
  }
  Glib::ustring file = originalFile();
  if (Glib::path_get_basename(file) == Glib::path_get_basename(ref.raw))
  {
    setInfo(M("TT_SERIESEXP_IS_REFERENCE"));
    return;
  }
  if (exposureIsAutomatic())
  {
    setInfo(M("TT_SERIESEXP_AUTOMATIC"));
    return;
  }

  cJSON* body = cJSON_CreateObject();
  cJSON_AddStringToObject(body, "session", sessionOf(ref).c_str());
  cJSON_AddStringToObject(body, "raw", file.c_str());
  cJSON_AddNumberToObject(body, "min_similarity", esbySettings().SeriesExpMinSimilarity);
  double ax, ay;
  if (readAfPoint(env->getVarAsString("Fname"), ax, ay))
  {
    cJSON* p = cJSON_AddArrayToObject(body, "af_point");
    cJSON_AddItemToArray(p, cJSON_CreateNumber(ax));
    cJSON_AddItemToArray(p, cJSON_CreateNumber(ay));
  }
  setInfo(M("TT_SERIESEXP_WAITING"));
  service->post("/measure", body, [this, ref, file, manual, retried](cJSON* answer, const Glib::ustring& error) {
    if (originalFile() != file)
      return; // another image was opened meanwhile
    cJSON* status = answer ? cJSON_GetObjectItem(answer, "status") : nullptr;
    if (!cJSON_IsString(status))
    {
      setInfo(Glib::ustring::compose(M("TT_SERIESEXP_SERVICE_ERROR"), error));
      return;
    }
    Glib::ustring st = status->valuestring;
    auto number = [answer](const char* name, double def) {
      cJSON* item = cJSON_GetObjectItem(answer, name);
      return cJSON_IsNumber(item) ? item->valuedouble : def;
    };

    // the service forgot the session (inactivity): the reference is set again silently, from the
    // raw file and the choice kept by the fork (the fingerprint is never kept), then measured again
    if ((st == "no_reference") && !retried)
    {
      cJSON* again = cJSON_CreateObject();
      cJSON_AddStringToObject(again, "session", sessionOf(ref).c_str());
      cJSON_AddStringToObject(again, "raw", ref.raw.c_str());
      if (ref.hasPoint)
      {
        cJSON* p = cJSON_AddArrayToObject(again, "point");
        cJSON_AddItemToArray(p, cJSON_CreateNumber(ref.px));
        cJSON_AddItemToArray(p, cJSON_CreateNumber(ref.py));
      }
      double rx, ry;
      if (readAfPoint(ref.raw, rx, ry))
      {
        cJSON* p = cJSON_AddArrayToObject(again, "af_point");
        cJSON_AddItemToArray(p, cJSON_CreateNumber(rx));
        cJSON_AddItemToArray(p, cJSON_CreateNumber(ry));
      }
      service->post("/reference", again, [this, file, manual](cJSON* a, const Glib::ustring&) {
        cJSON* s2 = a ? cJSON_GetObjectItem(a, "status") : nullptr;
        if ((originalFile() == file) && cJSON_IsString(s2) && (Glib::ustring(s2->valuestring) == "ok"))
          measureCurrent(manual, true);
        else if (originalFile() == file)
          setInfo(M("TT_SERIESEXP_NO_REFERENCE"));
      });
      return;
    }

    if (st == "ok")
    {
      double d = number("delta_ev", 0.0);
      double similarity = number("similarity", 0.0);
      double clipped = number("clipped_fraction", 0.0);
      double comp = clampComp(ref.comp + d + seriesOffset());
      Glib::ustring values = Glib::ustring::compose(M("TT_SERIESEXP_MEASURED"), ev(d),
                                                    Glib::ustring::format(std::fixed, std::setprecision(2), similarity),
                                                    Glib::ustring::format(std::fixed, std::setprecision(1), clipped * 100.0),
                                                    ev(comp));
      const EsbySettings& s = esbySettings();
      bool automatic = !manual && (follow != "none") && (similarity >= s.SeriesExpAutoSimilarity)
                    && (clipped <= s.SeriesExpMaxClipped) && (std::fabs(d) <= s.SeriesExpMaxEv);
      if (automatic)
      {
        storeDelta(d);
        recompute(false);
        setInfo(M("TT_SERIESEXP_AUTO_APPLIED") + "\n" + values);
      }
      else
        setSuggestion(d, M("TT_SERIESEXP_SUGGESTED") + "\n" + values);
      return;
    }

    // no face (person seen from the back): the AF area measure is only proposed, never applied
    cJSON* fallback = cJSON_GetObjectItem(answer, "fallback_af");
    if ((st == "no_face") && cJSON_IsObject(fallback))
    {
      cJSON* d = cJSON_GetObjectItem(fallback, "delta_ev");
      if (cJSON_IsNumber(d))
      {
        setSuggestion(d->valuedouble, Glib::ustring::compose(M("TT_SERIESEXP_SUGGESTED_AF"), ev(d->valuedouble),
                                                             ev(clampComp(ref.comp + d->valuedouble + seriesOffset()))));
        return;
      }
    }
    // no_match: another person is probably on the AF point, no fallback
    setInfo(Glib::ustring::compose(M(st == "no_match" ? "TT_SERIESEXP_NO_MATCH" : "TT_SERIESEXP_STATUS"), st));
  });
}

// ---------------------------------------------------------------------------------------

void TTSeriesExposure::setInfo(const Glib::ustring& action)
{
  Glib::ustring text = action;
  if (sequenceReady)
  {
    Glib::ustring refs;
    if (refSeries.valid)
      refs += Glib::ustring::compose(M("TT_SERIESEXP_REF_SERIES"), Glib::path_get_basename(refSeries.raw));
    if (refParent.valid)
      refs += (refs.empty() ? "" : "\n")
            + Glib::ustring::compose(M("TT_SERIESEXP_REF_PARENT"), Glib::path_get_basename(refParent.raw),
                                     Glib::path_get_basename(refParent.origin));
    text += (text.empty() ? "" : "\n") + sequenceText + (refs.empty() ? "" : "\n" + refs);
  }
  lbInfo->set_text(text);
  TT_LOG("TTSeriesExposure: %s\n", text.c_str());
}

void TTSeriesExposure::react(FakeProcEvent ev)
{
  if (!sharedConnected && (env != nullptr))
  {
    env->sharedVariables()->signalChanged().connect(sigc::mem_fun(*this, &TTSeriesExposure::sharedChanged));
    sharedConnected = true;
  }
  if ((ev == FakeEvExifTransmitted) && getExpander()->getEnabled())
    startSequence();
  // changed by hand while it was shown (image closed): it stops following its reference
  if ((ev == FakeEvImageClosed) && getExpander()->getEnabled() && stateLoaded && (follow != "none")
  && hasShownComp && (tonecurve != nullptr) && (std::fabs(currentExpComp() - shownComp) >= SERIESEXP_EPS)
  && (stateFile == originalFile()))
  {
    EsbyVarValue v;
    v.name = "follow";
    v.s = "none";
    env->sharedVariables()->getClient()->fileSet(stateFile, "exposure", {v}, nullptr);
  }
}

void TTSeriesExposure::adjusterChanged(Adjuster* a, double newval)
{
  if (a == adjOffset)
    offsetAdjusted();
}

// AF point of a Panasonic raw file, in 0 - 1 of the displayed image (EXIF orientation applied);
// false when absent or invalid (ex: 4194303.999 in manual focus on the S5 II)
bool TTSeriesExposure::readAfPoint(const Glib::ustring& file, double& x, double& y)
{
  try
  {
    auto image = Exiv2::ImageFactory::open(std::string(file));
    image->readMetadata();
    Exiv2::ExifData& exif = image->exifData();
    auto af = exif.findKey(Exiv2::ExifKey("Exif.Panasonic.AFPointPosition"));
    if ((af == exif.end()) || (af->count() < 2))
      return false;
    double ax = af->toFloat(0), ay = af->toFloat(1);
    if (!(ax >= 0.0 && ax <= 1.0 && ay >= 0.0 && ay <= 1.0))
      return false;
    int orientation = 1;
    auto o = exif.findKey(Exiv2::ExifKey("Exif.Image.Orientation"));
    if (o != exif.end())
      orientation = (int) o->toFloat(0);
    switch (orientation)
    {
      case 3: x = 1.0 - ax; y = 1.0 - ay; break;  // 180 degrees
      case 6: x = 1.0 - ay; y = ax; break;        // 90 degrees clockwise
      case 8: x = ay; y = 1.0 - ax; break;        // 90 degrees counterclockwise
      default: x = ax; y = ay; break;
    }
    return true;
  }
  catch (const std::exception&)
  {
    return false;
  }
}

Glib::ustring TTSeriesExposure::themeExport()
{
  return getToolName() + ":" + "active " + std::string(getExpander()->getEnabled() ? "1" : "0") + "\n";
}

void TTSeriesExposure::themeImport(std::ifstream& myfile)
{
  std::string line;
  bool condition = true;
  while (condition)
  {
    int position = myfile.tellg();
    condition = static_cast<bool>(getline(myfile, line));

    std::istringstream tokensplitter(line);
    std::string token;
    if (getline(tokensplitter, token, ':'))
    {
      if (token == getToolName())
      {
        std::string key, value;
        if (getline(tokensplitter, key, ' ') && getline(tokensplitter, value, ' '))
        {
          if (key == "active")
            getExpander()->setEnabled(value == "1");
        }
      }
      else
      {
        //we restore the position since it is a line for another tool that we read.
        myfile.seekg(position);
        condition = false;
      }
    }
  }
}
