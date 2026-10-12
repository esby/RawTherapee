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
static const char REFERENCE_VAR[] = "exposure.reference";

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

TTSeriesExposure::Reference TTSeriesExposure::Reference::parse(const Glib::ustring& text)
{
  Reference r;
  cJSON* json = cJSON_Parse(text.c_str());
  if (json == nullptr)
    return r;
  cJSON* raw = cJSON_GetObjectItem(json, "raw");
  cJSON* comp = cJSON_GetObjectItem(json, "comp");
  cJSON* gap = cJSON_GetObjectItem(json, "gap");
  cJSON* point = cJSON_GetObjectItem(json, "point");
  cJSON* target = cJSON_GetObjectItem(json, "target");
  if (cJSON_IsString(raw) && cJSON_IsNumber(comp))
  {
    r.valid = true;
    r.raw = raw->valuestring;
    r.comp = comp->valuedouble;
    r.gap = cJSON_IsNumber(gap) ? gap->valuedouble : 0.0;
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
  cJSON_AddNumberToObject(json, "gap", gap);
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
  stateLoaded = false;
  gap = 0.0;
  state = "default";
  generation = 0;
  hasSuggestion = false;
  suggestedGap = 0.0;
  sharedConnected = false;
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
  btReference = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_REFERENCE")));
  btReference->set_tooltip_text(M("TT_SERIESEXP_REFERENCE_TOOLTIP"));
  btNoReference = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_NO_REFERENCE_BUTTON")));
  btNoReference->set_tooltip_text(M("TT_SERIESEXP_NO_REFERENCE_TOOLTIP"));
  refBox->add(*tbPipette);
  refBox->add(*btReference);
  refBox->add(*btNoReference);
  pack_start(*refBox, Gtk::PACK_SHRINK, 0);

  // gap of the image
  adjGap = Gtk::manage(new Adjuster(M("TT_SERIESEXP_GAP"), -3.0, 3.0, 0.05, 0.0));
  adjGap->setAdjusterListener(this);
  adjGap->set_tooltip_text(M("TT_SERIESEXP_GAP_TOOLTIP"));
  pack_start(*adjGap, Gtk::PACK_SHRINK, 0);

  Gtk::FlowBox* imageBox = Gtk::manage(new Gtk::FlowBox());
  imageBox->set_selection_mode(Gtk::SELECTION_NONE);
  imageBox->set_column_spacing(4);
  imageBox->set_row_spacing(2);
  lbState = Gtk::manage(new Gtk::Label(""));
  btMeasure = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_MEASURE")));
  btMeasure->set_tooltip_text(M("TT_SERIESEXP_MEASURE_TOOLTIP"));
  btApply = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_APPLY")));
  btApply->set_tooltip_text(M("TT_SERIESEXP_APPLY_TOOLTIP"));
  btApply->set_sensitive(false);
  btReset = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_RESET")));
  btReset->set_tooltip_text(M("TT_SERIESEXP_RESET_TOOLTIP"));
  imageBox->add(*lbState);
  imageBox->add(*btMeasure);
  imageBox->add(*btApply);
  imageBox->add(*btReset);
  pack_start(*imageBox, Gtk::PACK_SHRINK, 0);

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
    if (hasSuggestion)
      setGap(suggestedGap, "computed", true);
  });
  btReset->signal_clicked().connect([this]() { setGap(0.0, "default", true); });
  tbPipette->signal_toggled().connect([this]() {
    if (tbPipette->get_active())
    {
      if (getEditProvider() != nullptr)
        subscribe();
    }
    else if (isCurrentSubscriber())
      unsubscribe();
  });
  showState();
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

// ESBY_ORIGIN (see TTSeriesWB): the original file of a copy opened by a launcher such as rt_queue
Glib::ustring TTSeriesExposure::originalFile()
{
  return EsbySharedVariables::originalFile(env);
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
  // a value for this image only (its gap with the reference): not shared by TTShare
  env->shareSuppressed++;
  if (listener)
    listener->panelChanged(EvExpComp, Glib::ustring::format(std::setw(5), std::fixed, std::setprecision(2), value));
  env->shareSuppressed--;
}


// automatic exposure or histogram matching: each image gets its own exposure, a gap does not hold
bool TTSeriesExposure::exposureIsAutomatic()
{
  ProcParams pp;
  tonecurve->write(&pp);
  return pp.toneCurve.autoexp || pp.toneCurve.histmatching;
}

// ---------------------------------------------------------------------------------------
// state of the image (esby server)
// ---------------------------------------------------------------------------------------

// an image is opened: its gap and state (server), then the reference of its series; an image without
// gap is measured at once with the service
void TTSeriesExposure::imageOpened()
{
  stateFile = originalFile();
  if (stateFile.empty())
  {
    setInfo(M("TT_SERIESEXP_NO_IMAGE"));
    return;
  }
  series = EsbySharedVariables::realPath(EsbySharedVariables::sequenceFolder(stateFile));
  env->sharedVariables()->setSequence(stateFile, Glib::ustring());
  int current = ++generation;
  gap = 0.0;
  state = "default";
  stateLoaded = false;
  reference = Reference();
  hasSuggestion = false;
  btApply->set_sensitive(false);
  showState();
  EsbyWBClient* client = env->sharedVariables()->getClient();
  if (!client->isConnected())
  {
    // the gap can be set by hand, it is not kept
    stateLoaded = true;
    setInfo(M("TT_SERIESEXP_NO_SERVER"));
    return;
  }
  client->fileGet(stateFile, "exposure", [this, current](bool ok, const std::vector<EsbyVarValue>& fields) {
    if (current != generation)
      return;
    for (const EsbyVarValue& f : fields)
    {
      if (f.name == "gap")
        gap = (f.type == RT_VARIABLE_TYPE_INT) ? f.i : f.d;
      else if ((f.name == "state") && ((f.s == "computed") || (f.s == "manual") || (f.s == "default")))
        state = f.s;
    }
    stateLoaded = true;
    showState();
    loadReference(ok && (state == "default"));
  });
}

// the reference of the series (set on its folder); measureAfter: an image without gap is measured
void TTSeriesExposure::loadReference(bool measureAfter)
{
  EsbyWBClient* client = env->sharedVariables()->getClient();
  if (series.empty() || !client->isConnected())
    return;
  int current = generation;
  client->varGet(series, "", [this, current, measureAfter](bool, const std::vector<EsbyVarValue>& vars) {
    if (current != generation)
      return;
    reference = Reference();
    for (const EsbyVarValue& v : vars)
      if ((v.name == REFERENCE_VAR) && (v.origin == series))
        reference = Reference::parse(v.s);
    setInfo("");
    if (measureAfter && reference.valid && !isReference() && service->isEnabled() && getExpander()->getEnabled())
      measureCurrent(false);
  });
}

bool TTSeriesExposure::isReference()
{
  return reference.valid && (Glib::path_get_basename(reference.raw) == Glib::path_get_basename(originalFile()));
}

// the gap of the image becomes g: its compensation moves by the difference (for this image only)
void TTSeriesExposure::setGap(double g, const Glib::ustring& newState, bool apply)
{
  if (apply && (tonecurve != nullptr) && (stateFile == originalFile()))
    setExpComp(clampComp(currentExpComp() + g - gap));
  gap = g;
  state = newState;
  hasSuggestion = false;
  btApply->set_sensitive(false);
  saveState();
  showState();
}

void TTSeriesExposure::saveState()
{
  EsbyWBClient* client = env->sharedVariables()->getClient();
  if (stateFile.empty() || !client->isConnected())
    return;
  EsbyVarValue g;
  g.name = "gap";
  g.type = RT_VARIABLE_TYPE_DOUBLE;
  g.d = gap;
  EsbyVarValue s;
  s.name = "state";
  s.s = state;
  client->fileSet(stateFile, "exposure", {g, s}, nullptr);
}

// the slider and the state label show the gap of the image
void TTSeriesExposure::showState()
{
  updatingWidgets = true;
  adjGap->block(true);
  adjGap->setValue(gap);
  adjGap->block(false);
  updatingWidgets = false;
  lbState->set_text(M(state == "computed" ? "TT_SERIESEXP_STATE_COMPUTED"
                      : state == "manual" ? "TT_SERIESEXP_STATE_MANUAL" : "TT_SERIESEXP_STATE_DEFAULT"));
}

// ---------------------------------------------------------------------------------------
// reference of the series
// ---------------------------------------------------------------------------------------

// the current image becomes the reference of its series: with the pipette, its brightness is the
// target; with the service, its face (at the point, near the AF point, or the largest)
void TTSeriesExposure::setReferenceHere(bool hasPoint, double px, double py, bool hasTarget, double target)
{
  if (series.empty() || (tonecurve == nullptr) || !env->sharedVariables()->isConnected())
  {
    setInfo(M("TT_SERIESEXP_NO_SERVER"));
    return;
  }
  Reference ref;
  ref.valid = true;
  ref.raw = originalFile();
  ref.comp = currentExpComp();
  ref.gap = gap;
  ref.hasPoint = hasPoint;
  ref.px = px;
  ref.py = py;
  ref.hasTarget = hasTarget;
  ref.target = target;
  if (!service->isEnabled())
  {
    publishReference(ref);
    return;
  }

  cJSON* body = cJSON_CreateObject();
  cJSON_AddStringToObject(body, "session", (series + "|" + Glib::path_get_basename(ref.raw)).c_str());
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
  service->post("/reference", body, [this, ref, file](cJSON* answer, const Glib::ustring& error) {
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
    publishReference(ref);
  });
}

void TTSeriesExposure::publishReference(const Reference& ref)
{
  EsbyVarValue value;
  value.name = REFERENCE_VAR;
  value.s = ref.serialize();
  env->sharedVariables()->getClient()->varSet(series, "", value, nullptr);
  reference = ref;
  setInfo(M("TT_SERIESEXP_REFERENCE_SET"));
}

void TTSeriesExposure::removeReference()
{
  if (!isReference())
  {
    setInfo(M("TT_SERIESEXP_NOT_A_REFERENCE"));
    return;
  }
  env->sharedVariables()->getClient()->varUnset(series, "", REFERENCE_VAR, nullptr);
  reference = Reference();
  setInfo(M("TT_SERIESEXP_REFERENCE_REMOVED"));
}

// ---------------------------------------------------------------------------------------
// pipette
// ---------------------------------------------------------------------------------------

bool TTSeriesExposure::button1Pressed(int modifierKey)
{
  if (!getExpander()->getEnabled() || (tonecurve == nullptr))
    return false;
  double luminance;
  if (!measure(luminance))
  {
    setInfo(M("TT_SERIESEXP_NO_VALUE"));
    return true;
  }

  // ctrl + click: this image becomes the reference of the series; its brightness is the target and,
  // with the service, the point designates the face
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
    setReferenceHere(hasPoint, px, py, true, luminance);
    return true;
  }

  // click: the gap with the reference, from the brightness of the same area.
  // brightness ~ scene * 2^compensation, so: gap = log2(target / brightness) - comp. of the reference
  // + comp. of this image + gap of the reference. The tone curve of the exposure tool makes it
  // approximate: a second click refines it.
  if (!reference.valid || !reference.hasTarget)
  {
    setInfo(M("TT_SERIESEXP_NO_TARGET"));
    return true;
  }
  if (exposureIsAutomatic())
  {
    setInfo(M("TT_SERIESEXP_AUTOMATIC"));
    return true;
  }
  double g = std::log2(reference.target / luminance) - reference.comp + currentExpComp() + reference.gap;
  setGap(g, "computed", true);
  setInfo(Glib::ustring::compose(M("TT_SERIESEXP_GAP_MEASURED"), ev(g)));
  return true;
}

// ---------------------------------------------------------------------------------------
// external measuring service
// ---------------------------------------------------------------------------------------

void TTSeriesExposure::setSuggestion(double g, const Glib::ustring& text)
{
  suggestedGap = g;
  hasSuggestion = true;
  btApply->set_sensitive(true);
  setInfo(text);
}

// measure of the current image by the service, against the face of the reference of its series:
// gap = delta_ev (raw data) + gap of the reference. manual: from the measure button (only proposed)
void TTSeriesExposure::measureCurrent(bool manual, bool retried)
{
  if (!service->isEnabled() || (tonecurve == nullptr) || !stateLoaded)
    return;
  Reference ref = reference;
  if (!ref.valid)
  {
    setInfo(M("TT_SERIESEXP_NO_REFERENCE"));
    return;
  }
  if (isReference())
  {
    setInfo(M("TT_SERIESEXP_IS_REFERENCE"));
    return;
  }
  if (exposureIsAutomatic())
  {
    setInfo(M("TT_SERIESEXP_AUTOMATIC"));
    return;
  }
  Glib::ustring file = originalFile();
  Glib::ustring session = series + "|" + Glib::path_get_basename(ref.raw);
  cJSON* body = cJSON_CreateObject();
  cJSON_AddStringToObject(body, "session", session.c_str());
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
  service->post("/measure", body, [this, ref, file, session, manual, retried](cJSON* answer, const Glib::ustring& error) {
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
      cJSON_AddStringToObject(again, "session", session.c_str());
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
      double g = number("delta_ev", 0.0) + ref.gap;
      double similarity = number("similarity", 0.0);
      double clipped = number("clipped_fraction", 0.0);
      Glib::ustring values = Glib::ustring::compose(M("TT_SERIESEXP_MEASURED"), ev(g),
                                                    Glib::ustring::format(std::fixed, std::setprecision(2), similarity),
                                                    Glib::ustring::format(std::fixed, std::setprecision(1), clipped * 100.0));
      const EsbySettings& s = esbySettings();
      bool automatic = !manual && (state != "manual") && (similarity >= s.SeriesExpAutoSimilarity)
                    && (clipped <= s.SeriesExpMaxClipped) && (std::fabs(g) <= s.SeriesExpMaxEv);
      if (automatic)
      {
        setGap(g, "computed", true);
        setInfo(M("TT_SERIESEXP_AUTO_APPLIED") + "\n" + values);
      }
      else
        setSuggestion(g, M("TT_SERIESEXP_SUGGESTED") + "\n" + values);
      return;
    }

    // no face (person seen from the back): the AF area measure is only proposed, never applied
    cJSON* fallback = cJSON_GetObjectItem(answer, "fallback_af");
    if ((st == "no_face") && cJSON_IsObject(fallback))
    {
      cJSON* d = cJSON_GetObjectItem(fallback, "delta_ev");
      if (cJSON_IsNumber(d))
      {
        double g = d->valuedouble + ref.gap;
        setSuggestion(g, Glib::ustring::compose(M("TT_SERIESEXP_SUGGESTED_AF"), ev(g)));
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
  if (!series.empty())
  {
    Glib::ustring ref = !reference.valid ? M("TT_SERIESEXP_REF_NONE")
                      : isReference() ? M("TT_SERIESEXP_REF_THIS")
                      : Glib::ustring::compose(M("TT_SERIESEXP_REF"), Glib::path_get_basename(reference.raw));
    text += (text.empty() ? "" : "\n") + Glib::ustring::compose(M("TT_SERIESEXP_SEQ"), Glib::path_get_basename(series)) + "\n" + ref;
  }
  lbInfo->set_text(text);
  TT_LOG("TTSeriesExposure: %s\n", text.c_str());
}

void TTSeriesExposure::react(FakeProcEvent ev)
{
  if (!sharedConnected && (env != nullptr))
  {
    // the reference changed (another instance, or the variable panel)
    env->sharedVariables()->signalChanged().connect([this]() {
      if (stateLoaded && getExpander()->getEnabled())
        loadReference(false);
    });
    sharedConnected = true;
  }
  if ((ev == FakeEvExifTransmitted) && getExpander()->getEnabled())
    imageOpened();
}

void TTSeriesExposure::adjusterChanged(Adjuster* a, double newval)
{
  if ((a == adjGap) && !updatingWidgets)
  {
    if (!getExpander()->getEnabled())
    {
      // the tool is off: it does nothing
      showState();
      return;
    }
    setGap(newval, "manual", true);
  }
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
