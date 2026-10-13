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
#include "ttserieswb.h"
#include "ttlog.h"
#include "esbyoptions.h"
#include "esbysharedvars.h"
#include "multilangmgr.h"
#include "variable.h"
#include "wbprovider.h"
#include "tools/whitebalance.h"
#include "rtengine/colortemp.h"
#include <cjson/cJSON.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace rtengine;
using namespace rtengine::procparams;

// limits of the white balance tool (tools/whitebalance.cc)
static const int SERIESWB_MINTEMP = 1500;
static const int SERIESWB_MAXTEMP = 60000;
static const double SERIESWB_MINGREEN = 0.02;
static const double SERIESWB_MAXGREEN = 100.0;
// blue/red equalizer: limits of the white balance tool (MINEQUAL, MAXEQUAL), 1.0 for the camera
static const double SERIESWB_MIN_EQUAL = 0.5;
static const double SERIESWB_MAX_EQUAL = 2.0;

// ranges of the adjusters
static const double SERIESWB_MIN_MIRED = -100.0;
static const double SERIESWB_MAX_MIRED = 100.0;
static const double SERIESWB_MIN_GREEN_FACTOR = 0.5;
static const double SERIESWB_MAX_GREEN_FACTOR = 2.0;

static const char REFERENCE_VAR[] = "wb.reference";

static Glib::ustring mireds(double value)
{
  if (std::fabs(value) < 0.05) // -0.0
    value = 0.0;
  return Glib::ustring::format(std::showpos, std::fixed, std::setprecision(1), value);
}

static Glib::ustring factor(double value)
{
  return Glib::ustring::format(std::fixed, std::setprecision(3), value);
}

static Glib::ustring speed(double t)
{
  return (t > 0.0) ? Glib::ustring::compose("1/%1", std::lround(1.0 / t)) : Glib::ustring("?");
}

// ---------------------------------------------------------------------------------------
// reference (JSON text of the wb.reference variable)
// ---------------------------------------------------------------------------------------

TTSeriesWB::Reference TTSeriesWB::Reference::parse(const Glib::ustring& text)
{
  Reference r;
  cJSON* json = cJSON_Parse(text.c_str());
  if (json == nullptr)
    return r;
  auto number = [json](const char* name, double def) {
    cJSON* item = cJSON_GetObjectItem(json, name);
    return cJSON_IsNumber(item) ? item->valuedouble : def;
  };
  cJSON* raw = cJSON_GetObjectItem(json, "raw");
  if (cJSON_IsString(raw))
  {
    r.valid = true;
    r.raw = raw->valuestring;
    r.shutter = number("shutter", 0.0);
    r.gap = number("gap", 0.0);
    r.k = number("k", 0.0);
    cJSON* hss = cJSON_GetObjectItem(json, "hss");
    r.hasHss = cJSON_IsNumber(hss);
    r.hss = r.hasHss ? hss->valuedouble : 0.0;
    double sync = number("sync", r.sync);
    if (sync > 0.0)
      r.sync = sync;
  }
  cJSON_Delete(json);
  return r;
}

Glib::ustring TTSeriesWB::Reference::serialize() const
{
  cJSON* json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "raw", raw.c_str());
  cJSON_AddNumberToObject(json, "shutter", shutter);
  cJSON_AddNumberToObject(json, "gap", gap);
  cJSON_AddNumberToObject(json, "k", k);
  if (hasHss)
    cJSON_AddNumberToObject(json, "hss", hss);
  cJSON_AddNumberToObject(json, "sync", sync);
  char* text = cJSON_PrintUnformatted(json);
  Glib::ustring result(text);
  cJSON_free(text);
  cJSON_Delete(json);
  return result;
}

// ---------------------------------------------------------------------------------------
// interface
// ---------------------------------------------------------------------------------------

TTSeriesWB::TTSeriesWB() : FoldableToolPanel(this, "TTSeriesWB", M("TT_SERIESWB_LABEL"), false, true)
{
  whitebalance = nullptr;
  stateLoaded = false;
  gap = 0.0;
  state = "none";
  generation = 0;
  sharedConnected = false;
  updatingWidgets = false;
  loading = false;

  // reference of the series
  Gtk::FlowBox* refBox = Gtk::manage(new Gtk::FlowBox());
  refBox->set_selection_mode(Gtk::SELECTION_NONE);
  refBox->set_column_spacing(4);
  refBox->set_row_spacing(2);
  btReference = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_REFERENCE")));
  btReference->set_tooltip_text(M("TT_SERIESWB_REFERENCE_TOOLTIP"));
  btNoReference = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_NO_REFERENCE_BUTTON")));
  btNoReference->set_tooltip_text(M("TT_SERIESWB_NO_REFERENCE_TOOLTIP"));
  btModel = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_MODEL")));
  btModel->set_tooltip_text(M("TT_SERIESWB_MODEL_TOOLTIP"));
  refBox->add(*btReference);
  refBox->add(*btNoReference);
  refBox->add(*btModel);
  pack_start(*refBox, Gtk::PACK_SHRINK, 0);

  // shift of the series: changed here like in the white balance tool, shared by TTShare
  // positive shift: lower temperature, cooler rendering (ex: 5044 K + 15.4 mireds = 4679 K)
//  adjMired = Gtk::manage(new Adjuster(M("TT_SERIESWB_MIRED"), SERIESWB_MIN_MIRED, SERIESWB_MAX_MIRED, 0.5, 0.0));
  adjMired = Gtk::manage(new Adjuster(M("TT_SERIESWB_MIRED"), SERIESWB_MIN_MIRED, SERIESWB_MAX_MIRED, 0.1, 0.0));
  adjMired->setAdjusterListener(this);
  adjMired->set_tooltip_text(M("TT_SERIESWB_MIRED_TOOLTIP"));
  pack_start(*adjMired, Gtk::PACK_SHRINK, 0);

//  adjGreen = Gtk::manage(new Adjuster(M("TT_SERIESWB_GREEN"), 0.8, 1.25, 0.001, 1.0));
  adjGreen = Gtk::manage(new Adjuster(M("TT_SERIESWB_GREEN"), SERIESWB_MIN_GREEN_FACTOR, SERIESWB_MAX_GREEN_FACTOR, 0.001, 1.0));
  adjGreen->setAdjusterListener(this);
  adjGreen->set_tooltip_text(M("TT_SERIESWB_GREEN_TOOLTIP"));
  pack_start(*adjGreen, Gtk::PACK_SHRINK, 0);

  adjEqual = Gtk::manage(new Adjuster(M("TT_SERIESWB_EQUAL"), SERIESWB_MIN_EQUAL, SERIESWB_MAX_EQUAL, 0.001, 1.0));
  adjEqual->setAdjusterListener(this);
  adjEqual->set_tooltip_text(M("TT_SERIESWB_EQUAL_TOOLTIP"));
  pack_start(*adjEqual, Gtk::PACK_SHRINK, 0);

  // gap of the image
  adjGap = Gtk::manage(new Adjuster(M("TT_SERIESWB_GAP"), SERIESWB_MIN_MIRED, SERIESWB_MAX_MIRED, 0.1, 0.0));
  adjGap->setAdjusterListener(this);
  adjGap->set_tooltip_text(M("TT_SERIESWB_GAP_TOOLTIP"));
  pack_start(*adjGap, Gtk::PACK_SHRINK, 0);

//  Gtk::HBox* buttonBox = Gtk::manage(new Gtk::HBox());
  // flow box: the buttons go to the next line when the side panel is narrow
  Gtk::FlowBox* imageBox = Gtk::manage(new Gtk::FlowBox());
  imageBox->set_selection_mode(Gtk::SELECTION_NONE);
  imageBox->set_column_spacing(4);
  imageBox->set_row_spacing(2);
  lbState = Gtk::manage(new Gtk::Label(""));
  btCompute = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_COMPUTE")));
  btCompute->set_tooltip_text(M("TT_SERIESWB_COMPUTE_TOOLTIP"));
  btReset = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_RESET")));
  btReset->set_tooltip_text(M("TT_SERIESWB_RESET_TOOLTIP"));
  imageBox->add(*lbState);
  imageBox->add(*btCompute);
  imageBox->add(*btReset);
  pack_start(*imageBox, Gtk::PACK_SHRINK, 0);

  // high speed sync: from this shutter speed up (kept with the reference of the series)
  adjSync = Gtk::manage(new Adjuster(M("TT_SERIESWB_SYNC"), 30.0, 1000.0, 10.0, 250.0));
  adjSync->setAdjusterListener(this);
  adjSync->set_tooltip_text(M("TT_SERIESWB_SYNC_TOOLTIP"));
  pack_start(*adjSync, Gtk::PACK_SHRINK, 0);

  cbFlashOnly = Gtk::manage(new Gtk::CheckButton(M("TT_SERIESWB_FLASH_ONLY")));
  cbFlashOnly->set_tooltip_text(M("TT_SERIESWB_FLASH_ONLY_TOOLTIP"));
  pack_start(*cbFlashOnly, Gtk::PACK_SHRINK, 0);

  lbInfo = Gtk::manage(new Gtk::Label(""));
  lbInfo->set_line_wrap(true);
  lbInfo->set_line_wrap_mode(Pango::WRAP_WORD_CHAR); // the paths have long parts without spaces
  lbInfo->set_xalign(0.0);
//  pack_start(*lbInfo, Gtk::PACK_SHRINK, 0);

  // a label does not receive the clicks: it is put in an event box
  ebInfo = Gtk::manage(new Gtk::EventBox());
  ebInfo->add(*lbInfo);
  ebInfo->set_tooltip_text(M("TT_SERIESWB_COPY_TOOLTIP"));
  ebInfo->add_events(Gdk::BUTTON_PRESS_MASK);
  ebInfo->signal_button_press_event().connect([this](GdkEventButton* event) {
    if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1) && !lbInfo->get_text().empty())
    {
      Gtk::Clipboard::get()->set_text(lbInfo->get_text());
      TT_LOG("TTSeriesWB: status line copied to the clipboard\n");
    }
    return true;
  });
  ebInfo->signal_realize().connect([this]() {
    Glib::RefPtr<Gdk::Window> window = ebInfo->get_window();
    if (window)
      window->set_cursor(Gdk::Cursor::create(ebInfo->get_display(), "pointer"));
  });
  pack_start(*ebInfo, Gtk::PACK_SHRINK, 0);

  btReference->signal_clicked().connect([this]() { setReferenceHere(); });
  btNoReference->signal_clicked().connect([this]() { removeReference(); });
  btModel->signal_clicked().connect([this]() { modelFromImage(); });
  btCompute->signal_clicked().connect([this]() { computeAndApply(true); });
  btReset->signal_clicked().connect([this]() { setGap(0.0, "none", true); setInfo(M("TT_SERIESWB_GAP_RESET")); });
  cbFlashOnly->signal_toggled().connect([this]() {
    if (loading)
      return;
    esbySettings().SeriesWBFlashOnly = cbFlashOnly->get_active();
    showState();
    setInfo("");
  });

  // the values of the last session; a ttp profile loaded afterwards has the priority
  loading = true;
  cbFlashOnly->set_active(esbySettings().SeriesWBFlashOnly);
  getExpander()->setEnabled(esbySettings().SeriesWBEnabled);
  loading = false;
  showState();
}

TTSeriesWB::~TTSeriesWB()
{
  paramsConnection.disconnect();
  if (env != nullptr)
    env->shareKeyFilter = nullptr;
}

void TTSeriesWB::deploy()
{
  FoldableToolPanel::deploy();

  for (size_t i = 0; i < env->getToolPanels().size(); i++)
  {
    ToolPanel* p = static_cast<ToolPanel*>(env->getPanel(i));
    if ((p != nullptr) && (p->getToolName() == "whitebalance"))
      whitebalance = static_cast<WhiteBalance*>(p);
  }

  // a photo without flash takes no part in the white balance of the series (TTShare)
  env->shareKeyFilter = [this](const std::string& key) {
    if (key.compare(0, 14, "White Balance/") != 0)
      return true;
    return !getExpander()->getEnabled() || takesPart();
  };
}

void TTSeriesWB::deployLate()
{
}

// ESBY_ORIGIN: the original file of a copy opened by a launcher such as rt_queue
Glib::ustring TTSeriesWB::originalFile()
{
  return EsbySharedVariables::originalFile(env);
}

// Exif.Photo.Flash, as printed by Exiv2: "Fired", "Fired, return light detected", "Yes, compulsory",
// "No, compulsory", "Off, Did not fire"... The Panasonic maker note gives FlashFired ("Yes"/"No").
bool TTSeriesWB::flashFired()
{
  Glib::ustring flash = env->getExifVariable("Exif:Flash");
  if (!flash.empty())
    return (flash.find("Fired") != Glib::ustring::npos)
        || (flash.compare(0, 3, "Yes") == 0)
        || (flash.compare(0, 2, "On") == 0);

  Glib::ustring fired = env->getExifVariable("Exif:MakerNote:FlashFired");
  return (fired == "Yes");
}

bool TTSeriesWB::takesPart()
{
  return !cbFlashOnly->get_active() || ((env != nullptr) && flashFired());
}

double TTSeriesWB::shutter()
{
  return (env != nullptr) ? env->getVarAsDouble("SpeedValue") : 0.0;
}

bool TTSeriesWB::getCurrentWB(WBParams& wb)
{
  if (whitebalance == nullptr)
    return false;
  ProcParams pp;
  whitebalance->write(&pp);
  wb = pp.wb;
  return true;
}

// the "Camera" white balance of RawTherapee for the current image
bool TTSeriesWB::getCameraWB(const WBParams& wb, double& temp, double& green)
{
  WBProvider* provider = env->getWBProvider();
  if (provider == nullptr)
    return false;
  temp = -1.0;
  green = -1.0;
  provider->getCamWB(temp, green, wb.observer);
  return (temp > 0.0) && (green > 0.0); // nothing is returned when no image is loaded
}

// "Camera" method: the engine applies the camera multipliers, the temperature and tint shown by the white
// balance tool may still be the values of the profile: the image has the camera white balance then.
// The automatic methods are not handled (false).
bool TTSeriesWB::relativeWB(double& mired, double& green, double& equal)
{
  WBParams wb;
  double camTemp, camGreen;
  if ((env == nullptr) || !getCurrentWB(wb) || !getCameraWB(wb, camTemp, camGreen))
    return false;
  if (!wb.enabled || (wb.method == "Camera"))
  {
    mired = 0.0;
    green = 1.0;
    equal = 1.0;
    return true;
  }
  if ((wb.method != "Custom") || (wb.temperature <= 0))
    return false;
  mired = 1000000.0 / wb.temperature - 1000000.0 / camTemp;
  green = wb.green / camGreen;
  equal = wb.equal;
  return true;
}

// sets the white balance of the image: "Custom" method, temperature, tint and blue/red equalizer.
// WhiteBalance::setWB() only takes the temperature and the tint: the parameters (equalizer included)
// are read by the tool first, then setWB() keeps the equalizer and notifies the change.
// share: a change of the series (TTShare shares it); otherwise the gap of this image only
void TTSeriesWB::setRelativeWB(double mired, double green, double equal, bool share)
{
  WBParams wb;
  double camTemp, camGreen;
  if (!getCurrentWB(wb) || !getCameraWB(wb, camTemp, camGreen))
    return;
  double m = 1000000.0 / camTemp + mired;
  double t = (m > 0.0) ? 1000000.0 / m : SERIESWB_MAXTEMP;
  int temp = (int) std::lround(std::max<double>(SERIESWB_MINTEMP, std::min<double>(SERIESWB_MAXTEMP, t)));
  double g = std::max(SERIESWB_MINGREEN, std::min(SERIESWB_MAXGREEN, camGreen * green));
  double e = std::max(SERIESWB_MIN_EQUAL, std::min(SERIESWB_MAX_EQUAL, equal));

  ProcParams pp;
  whitebalance->write(&pp);
  pp.wb.enabled = true;
  pp.wb.method = "Custom";
  pp.wb.temperature = temp;
  pp.wb.green = g;
  pp.wb.equal = e;
  whitebalance->read(&pp);
  if (!share)
    env->shareSuppressed++;
  whitebalance->setWB(temp, g);
  if (!share)
    env->shareSuppressed--;
}

// ---------------------------------------------------------------------------------------
// state of the image (esby server)
// ---------------------------------------------------------------------------------------

// an image is opened: its gap and state (server), then the reference of its series; an image without
// a manual gap gets the gap computed from the reference
void TTSeriesWB::imageOpened()
{
  stateFile = originalFile();
  if (stateFile.empty())
  {
    setInfo(M("TT_SERIESWB_NO_IMAGE"));
    return;
  }
  series = EsbySharedVariables::seriesFolder(stateFile);
  int current = ++generation;
  gap = 0.0;
  state = "none";
  stateLoaded = false;
  reference = Reference();
  showState();
  EsbyWBClient* client = env->sharedVariables()->getClient();
  if (!client->isConnected())
  {
    // the gap can be set by hand, it is not kept
    stateLoaded = true;
    setInfo(M("TT_SERIESWB_NO_SERVER"));
    return;
  }
  client->fileGet(stateFile, "wb", [this, current](bool ok, const std::vector<EsbyVarValue>& fields) {
    if (current != generation)
      return;
    for (const EsbyVarValue& f : fields)
    {
      if (f.name == "gap")
        gap = (f.type == RT_VARIABLE_TYPE_INT) ? f.i : f.d;
      else if ((f.name == "state") && ((f.s == "computed") || (f.s == "manual") || (f.s == "none")))
        state = f.s;
    }
    stateLoaded = true;
    showState();
    loadReference(ok);
  });
}

// the reference of the series (set on its folder); computeAfter: the gap of the image follows it
void TTSeriesWB::loadReference(bool computeAfter)
{
  EsbyWBClient* client = env->sharedVariables()->getClient();
  if (series.empty() || !client->isConnected())
    return;
  int current = generation;
  client->varGet(series, "", [this, current, computeAfter](bool, const std::vector<EsbyVarValue>& vars) {
    if (current != generation)
      return;
    reference = Reference();
    for (const EsbyVarValue& v : vars)
      if ((v.name == REFERENCE_VAR) && (v.origin == series))
        reference = Reference::parse(v.s);
    showState();
    setInfo("");
    if (computeAfter && getExpander()->getEnabled() && (state != "manual"))
      computeAndApply(false);
  });
}

bool TTSeriesWB::isReference()
{
  return reference.valid && (Glib::path_get_basename(reference.raw) == Glib::path_get_basename(originalFile()));
}

// gap of the image from the reference: its own gap in high speed sync, otherwise the gap of the reference
// plus the ambient light, proportional to the difference of shutter time
bool TTSeriesWB::computeGap(double& g, Glib::ustring& why)
{
  if (!reference.valid)
  {
    why = M("TT_SERIESWB_NO_REFERENCE");
    return false;
  }
  if (isReference())
  {
    why = M("TT_SERIESWB_IS_REFERENCE");
    return false;
  }
  double t = shutter();
  if (reference.isHss(t))
  {
    if (!reference.hasHss)
    {
      why = M("TT_SERIESWB_NEED_HSS");
      return false;
    }
    g = reference.hss;
    why = Glib::ustring::compose(M("TT_SERIESWB_COMPUTED_HSS"), mireds(g));
    return true;
  }
  g = reference.gap;
  if ((t > 0.0) && (reference.shutter > 0.0))
    g += reference.k * (t - reference.shutter);
  g = std::max(SERIESWB_MIN_MIRED, std::min(SERIESWB_MAX_MIRED, g));
  why = Glib::ustring::compose(M("TT_SERIESWB_COMPUTED"), mireds(g), speed(t), speed(reference.shutter));
  return true;
}

// explicitly: the button (a manual gap is replaced); otherwise the image opened or the reference changed
void TTSeriesWB::computeAndApply(bool explicitly)
{
  if (!getExpander()->getEnabled() || !stateLoaded)
  {
    if (explicitly)
      setInfo(M("TT_SERIESWB_OFF"));
    return;
  }
  if (!takesPart())
  {
    setInfo(M("TT_SERIESWB_NO_FLASH"));
    return;
  }
  if (!explicitly && (state == "manual"))
    return;
  double g;
  Glib::ustring why;
  if (!computeGap(g, why))
  {
    setInfo(why);
    return;
  }
  if ((state != "computed") || (std::fabs(g - gap) >= 0.05))
    setGap(g, "computed", true);
  setInfo(why);
}

// the gap of the image becomes g: its white balance moves by the difference (for this image only)
void TTSeriesWB::setGap(double g, const Glib::ustring& newState, bool apply)
{
  if (apply && (stateFile == originalFile()) && (std::fabs(g - gap) >= 0.01))
  {
    double m, gr, e;
    if (relativeWB(m, gr, e))
      setRelativeWB(m + g - gap, gr, e, false);
    else
    {
      setInfo(M("TT_SERIESWB_NOT_HANDLED"));
      return;
    }
  }
  gap = g;
  state = newState;
  saveState();
  showState();
}

void TTSeriesWB::saveState()
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
  client->fileSet(stateFile, "wb", {g, s}, nullptr);
}

// the sliders show the shift of the series (the white balance of the image without its gap) and the gap
void TTSeriesWB::showState()
{
  updatingWidgets = true;
  adjGap->block(true);
  adjGap->setValue(gap);
  adjGap->block(false);
  double m, gr, e;
  bool known = relativeWB(m, gr, e);
  adjMired->block(true);
  adjGreen->block(true);
  adjEqual->block(true);
  adjMired->setValue(known ? m - gap : 0.0);
  adjGreen->setValue(known ? gr : 1.0);
  adjEqual->setValue(known ? e : 1.0);
  adjMired->block(false);
  adjGreen->block(false);
  adjEqual->block(false);
  adjSync->block(true);
  adjSync->setValue(std::lround(1.0 / reference.sync));
  adjSync->block(false);
  updatingWidgets = false;
  bool part = takesPart();
  adjMired->set_sensitive(known && part);
  adjGreen->set_sensitive(known && part);
  adjEqual->set_sensitive(known && part);
  adjGap->set_sensitive(part);
  lbState->set_text(M(state == "computed" ? "TT_SERIESWB_STATE_COMPUTED"
                      : state == "manual" ? "TT_SERIESWB_STATE_MANUAL" : "TT_SERIESWB_STATE_NONE"));
}

// ---------------------------------------------------------------------------------------
// reference of the series
// ---------------------------------------------------------------------------------------

// the current image becomes the reference of its series (normal flash): the model (coefficient, high
// speed sync) is kept
void TTSeriesWB::setReferenceHere()
{
  if (series.empty() || !env->sharedVariables()->isConnected())
  {
    setInfo(M("TT_SERIESWB_NO_SERVER"));
    return;
  }
  double t = shutter();
  if (reference.isHss(t))
  {
    setInfo(M("TT_SERIESWB_REFERENCE_HSS"));
    return;
  }
  Reference ref = reference;
  ref.valid = true;
  ref.raw = originalFile();
  ref.shutter = t;
  ref.gap = gap;
  publishReference(ref, Glib::ustring::compose(M("TT_SERIESWB_REFERENCE_SET"), speed(t), mireds(gap)));
}

void TTSeriesWB::publishReference(const Reference& ref, const Glib::ustring& message)
{
  EsbyVarValue value;
  value.name = REFERENCE_VAR;
  value.s = ref.serialize();
  env->sharedVariables()->getClient()->varSet(series, "", value, nullptr);
  reference = ref;
  showState();
  setInfo(message);
}

void TTSeriesWB::removeReference()
{
  if (!isReference())
  {
    setInfo(M("TT_SERIESWB_NOT_A_REFERENCE"));
    return;
  }
  env->sharedVariables()->getClient()->varUnset(series, "", REFERENCE_VAR, nullptr);
  reference = Reference();
  showState();
  setInfo(M("TT_SERIESWB_REFERENCE_REMOVED"));
}

// the gap set by hand on this image completes the model of the series: in high speed sync, it becomes the
// gap of the high speed sync; otherwise, with a shutter time clearly different from the reference, it
// gives the coefficient of the exposure (ambient light)
void TTSeriesWB::modelFromImage()
{
  // no reference yet: this image (normal flash) becomes it, the starting point of the model
  if (!reference.valid)
  {
    if (reference.isHss(shutter()))
      setInfo(M("TT_SERIESWB_MODEL_NEED_REFERENCE"));
    else
      setReferenceHere();
    return;
  }
  if (isReference())
  {
    setInfo(M("TT_SERIESWB_IS_REFERENCE"));
    return;
  }
  Reference ref = reference;
  double t = shutter();
  Glib::ustring message;
  if (ref.isHss(t))
  {
    ref.hasHss = true;
    ref.hss = gap;
    message = Glib::ustring::compose(M("TT_SERIESWB_MODEL_HSS"), mireds(gap));
  }
  else if ((t > 0.0) && (ref.shutter > 0.0) && (std::fabs(t - ref.shutter) > std::max(0.002, 0.2 * ref.shutter)))
  {
    ref.k = std::max(-2000.0, std::min(2000.0, (gap - ref.gap) / (t - ref.shutter)));
    message = Glib::ustring::compose(M("TT_SERIESWB_MODEL_COEF"), mireds(ref.k), speed(t), speed(ref.shutter));
  }
  else
  {
    setInfo(Glib::ustring::compose(M("TT_SERIESWB_MODEL_SAME_SPEED"), speed(t), speed(ref.shutter)));
    return;
  }
  // the gap of this image is the one of the model now
  state = "computed";
  saveState();
  publishReference(ref, message);
  observe("model");
}

// ---------------------------------------------------------------------------------------

void TTSeriesWB::setInfo(const Glib::ustring& action)
{
  Glib::ustring text = action;
  if (!series.empty())
  {
    Glib::ustring ref;
    if (!reference.valid)
      ref = M("TT_SERIESWB_REF_NONE");
    else
    {
      ref = isReference() ? M("TT_SERIESWB_REF_THIS")
          : Glib::ustring::compose(M("TT_SERIESWB_REF"), Glib::path_get_basename(reference.raw));
      ref += " " + Glib::ustring::compose(M("TT_SERIESWB_REF_MODEL"), speed(reference.shutter), mireds(reference.gap),
                                          mireds(reference.k),
                                          reference.hasHss ? mireds(reference.hss) : Glib::ustring("?"),
                                          speed(reference.sync));
    }
    text += (text.empty() ? "" : "\n") + Glib::ustring::compose(M("TT_SERIESWB_SEQ"), Glib::path_get_basename(series)) + "\n" + ref;
  }
  WBParams wb;
  double camTemp, camGreen;
  if ((env != nullptr) && getCurrentWB(wb) && getCameraWB(wb, camTemp, camGreen))
  {
    text += "\n" + Glib::ustring::compose(M("TT_SERIESWB_VALUES"),
                                          wb.temperature, factor(wb.green),
                                          (int) std::lround(camTemp), factor(camGreen),
                                          wb.method,
                                          (wb.observer == rtengine::StandardObserver::TEN_DEGREES) ? "10" : "2",
                                          factor(wb.equal));
    if (!takesPart())
      text += "\n" + M("TT_SERIESWB_NO_FLASH");
  }
  lbInfo->set_text(text);
  TT_LOG("TTSeriesWB: %s\n", text.c_str());
}

// turned on with an image already opened: it is handled from now on
void TTSeriesWB::enabledChanged()
{
  if (loading)
    return;
  esbySettings().SeriesWBEnabled = getExpander()->getEnabled();
  if (env == nullptr) // the tool is not deployed yet (settings loaded in the constructor)
    return;
  if (getExpander()->getEnabled() && !originalFile().empty())
    imageOpened();
}

void TTSeriesWB::adjusterChanged(Adjuster* a, double newval)
{
  if (updatingWidgets)
    return;
  if ((a == adjMired) || (a == adjGreen) || (a == adjEqual))
  {
    // a change of the white balance of the image, not of its gap: not suppressed, TTShare shares it
    double m, gr, e;
    if (!relativeWB(m, gr, e))
    {
      setInfo(M("TT_SERIESWB_NOT_HANDLED"));
      return;
    }
    setRelativeWB(adjMired->getValue() + gap, adjGreen->getValue(), adjEqual->getValue(), true);
    return;
  }
  if (a == adjGap)
  {
    if (!getExpander()->getEnabled())
    {
      // the tool is off: it does nothing
      showState();
      return;
    }
    setGap(newval, "manual", true);
    return;
  }
  if (a == adjSync)
  {
    double sync = 1.0 / std::max(1.0, newval);
    if (reference.valid && env->sharedVariables()->isConnected())
    {
      Reference ref = reference;
      ref.sync = sync;
      publishReference(ref, Glib::ustring::compose(M("TT_SERIESWB_SYNC_SET"), speed(sync)));
    }
    else
      reference.sync = sync; // without reference: only for this image
  }
}

// the lens names of some cameras end with many spaces
static Glib::ustring trimmed(const Glib::ustring& text)
{
  Glib::ustring::size_type end = text.find_last_not_of(" \t");
  return (end == Glib::ustring::npos) ? Glib::ustring() : text.substr(0, end + 1);
}

// sends to the server the white balance of the current image (kind: model, saved-..., closed-...) with
// its exposure, for the analysis of the link between the white balance and the exposure
void TTSeriesWB::observe(const Glib::ustring& kind)
{
  EsbyWBClient* client = env->sharedVariables()->getClient();
  Glib::ustring file = originalFile();
  WBParams wb;
  double camTemp, camGreen;
  if (!client->isConnected() || file.empty() || !getCurrentWB(wb) || !getCameraWB(wb, camTemp, camGreen) || !wb.enabled)
    return;
  file = Glib::build_filename(EsbySharedVariables::realPath(Glib::path_get_dirname(file)), Glib::path_get_basename(file));
  if (wb.method == "Camera") // the engine applies the camera white balance (see relativeWB)
  {
    wb.temperature = (int) std::lround(camTemp);
    wb.green = camGreen;
    wb.equal = 1.0;
  }
  if (wb.temperature <= 0)
    return;

//  double iso = env->getVarAsDouble("Iso");
  // the ISO is transmitted as an integer: read as a double, it gave 0 (and no light value)
  double iso = env->getVarAsInt("Iso");
  double fnumber = env->getVarAsDouble("FnumValue");
  double shutter = env->getVarAsDouble("SpeedValue");
  double mired = 1000000.0 / wb.temperature - 1000000.0 / camTemp;
  std::map<std::string, double> numbers = {
    {"mired", mired},
    {"green", wb.green / camGreen},
    {"equal", wb.equal},
    {"image_T", (double) wb.temperature}, {"image_G", wb.green},
    {"camera_T", camTemp}, {"camera_G", camGreen},
    {"series_mired", mired - gap}, {"series_green", wb.green / camGreen}, {"series_equal", wb.equal},
    {"gap", gap},
    {"iso", iso}, {"fnumber", fnumber}, {"shutter", shutter}};
  // light value of the settings (as exiftool computes it): log2(N^2 / t) - log2(ISO / 100)
  if ((iso > 0.0) && (fnumber > 0.0) && (shutter > 0.0))
    numbers["light_value"] = std::log2(fnumber * fnumber / shutter) - std::log2(iso / 100.0);
  std::map<std::string, Glib::ustring> texts = {
    {"kind", kind}, {"method", wb.method}, {"state", state},
    {"reference", reference.valid ? reference.serialize() : Glib::ustring()},
    {"source", series}, {"series_root", series},
    {"camera", env->getVarAsString("Camera")}, {"lens", trimmed(env->getVarAsString("Lens"))},
    {"flash", env->getExifVariable("Exif:Flash")}};
  client->observe(file, numbers, texts);
}

void TTSeriesWB::react(FakeProcEvent ev)
{
  if (!sharedConnected && (env != nullptr))
  {
    // the white balance changed (white balance tool, change received from the series): the sliders follow
    paramsConnection = env->paramsChangedSignal.connect([this](const ProcParams&, const ProcEvent&, bool) {
      if (!updatingWidgets)
        showState();
    });
    // the reference changed (another instance, or the variable panel)
    env->sharedVariables()->signalChanged().connect([this]() {
      if (stateLoaded && getExpander()->getEnabled())
        loadReference(true);
    });
    sharedConnected = true;
  }
  // FakeEvExifTransmitted: the exif data (flash) and the file name are known, TTTweaker has
  // already reacted (it is registered before this tool).
  if ((ev == FakeEvExifTransmitted) && getExpander()->getEnabled())
    imageOpened();

  // the image is saved, or closed (the profile is saved then, ex: rt_queue, one instance per image):
  // its white balance is the final one, observed for the analysis
  if (((ev == FakeEvFileSaved) || (ev == FakeEvImageClosed)) && getExpander()->getEnabled() && takesPart())
  {
    WBParams wb;
    if (getCurrentWB(wb))
    {
      Glib::ustring event = (ev == FakeEvFileSaved) ? "saved" : "closed";
      observe(event + "-" + ((wb.method == "Camera") ? Glib::ustring("camera") : state));
    }
  }
}

Glib::ustring TTSeriesWB::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(getExpander()->getEnabled() ? "1" : "0");
  Glib::ustring s_flash = getToolName() + ":" + "flash_only " + std::string(cbFlashOnly->get_active() ? "1" : "0");

//  return s_active + "\n" + s_mired + "\n" + s_green + "\n" + s_flash + "\n" + s_learn_tint + "\n";
  // the shift of the series and the gaps are working values, shared by the server: not in the ttp profiles
  return s_active + "\n" + s_flash + "\n";
}

void TTSeriesWB::themeImport(std::ifstream& myfile)
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
          // ignored: the "mired", "green" and "learn_tint" lines of the older profiles
/*
          else if (key == "mired")
          {
            adjMired->block(true);
            exactMired = Glib::Ascii::strtod(value);
            adjMired->setValue(exactMired);
            adjMired->block(false);
          }
          else if (key == "green")
          {
            adjGreen->block(true);
            exactGreen = Glib::Ascii::strtod(value);
            adjGreen->setValue(exactGreen);
            adjGreen->block(false);
          }
*/
          else if (key == "flash_only")
            cbFlashOnly->set_active(value == "1");
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

// ---------------------------------------------------------------------------------------
// commented lines of the former implementation (shift per folder with inheritance, learn, auto mode),
// kept as they were
// ---------------------------------------------------------------------------------------
//
// constructor:
//  Gtk::HBox* serverBox = Gtk::manage(new Gtk::HBox());
//  btSetParent = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_SET_PARENT")));
//  btSetParent->set_tooltip_text(M("TT_SERIESWB_SET_PARENT_TOOLTIP"));
//
// currentFolder():
//  if (env == nullptr)
//    return "";
//  Glib::ustring file = currentFile();
//  return file.empty() ? Glib::ustring() : realPath(Glib::path_get_dirname(file));
//
// setFolderValue():
//    folder = Glib::path_get_dirname(folder);
//
// sameWB():
//  return (std::abs(t1 - t2) <= 1) && (std::fabs(g1 - g2) <= 0.001);
//  return (std::fabs(1000000.0 / t1 - 1000000.0 / t2) <= 0.15) && (std::fabs(g1 / g2 - 1.0) <= 0.002);
//  return (std::abs(t1 - t2) <= 1) && (std::fabs(g1 / g2 - 1.0) <= 0.001);
//
// applyToCurrentImage():
//  setInfo(Glib::ustring::compose(M("TT_SERIESWB_APPLIED"),
//                                 (int) std::lround(camTemp), temp,
//                                 Glib::ustring::format(std::fixed, std::setprecision(1), exactMired))
//          + (origin.empty() ? Glib::ustring() : "\n" + origin));
//
// learnFromCurrentImage():
//  double green = wb.green / camGreen;
//  mired = std::round(mired * 10.0) / 10.0;
//  green = std::round(green * 1000.0) / 1000.0;
//    Glib::ustring folder = currentFolder();
