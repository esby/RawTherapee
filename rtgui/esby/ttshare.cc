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
#include "ttshare.h"
#include "ttlog.h"
#include "esbyoptions.h"
#include "multilangmgr.h"
#include "variable.h"
#include "esbysharedvars.h"
#include "options.h"
#include "rtimage.h"
#include "ppversion.h"
#include "wbprovider.h"
#include <iomanip>
#include "rtengine/procparams.h"
#include "rtengine/procevents.h"
#include <cmath>
#include <functional>
#include <memory>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <glib/gstdio.h>

using namespace rtengine;
using namespace rtengine::procparams;

// sections of the pp3 that can be shared: name in the pp3, label
static const std::vector<std::pair<std::string, const char*>> SHARE_SECTIONS = {
  {"Exposure", "TT_SHARE_SECTION_EXPOSURE"},
  {"White Balance", "TT_SHARE_SECTION_WB"},
  {"RGB Curves", "TT_SHARE_SECTION_RGB_CURVES"},
  {"Shadows & Highlights", "TT_SHARE_SECTION_SH"},
};

// keys shared in a section (a section not listed: all its keys). White balance: its method, temperature,
// tint and blue/red equalizer (not the settings of the automatic methods)
static const std::map<std::string, std::set<std::string>> SHARE_KEYS = {
  {"White Balance", {"Setting", "Temperature", "Green", "Equal"}},
};

// scale of the shift of a number: the temperature in mireds (1000000 / K, the same shift looks the same
// at any temperature), the tint and the equalizer as factors (logarithm), the others as they are
enum class ShareScale { Linear, Mired, Factor };
static ShareScale scaleOf(const std::string& key)
{
  if (key == "White Balance/Temperature")
    return ShareScale::Mired;
  if ((key == "White Balance/Green") || (key == "White Balance/Equal"))
    return ShareScale::Factor;
  return ShareScale::Linear;
}

// a value in the scale of its shifts, and back; false for a value out of the scale (0 K)
static bool toScale(const std::string& key, double value, double& scaled)
{
  switch (scaleOf(key))
  {
    case ShareScale::Mired:  if (value <= 0.0) return false; scaled = 1000000.0 / value; return true;
    case ShareScale::Factor: if (value <= 0.0) return false; scaled = std::log(value); return true;
    default:                 scaled = value; return true;
  }
}

static double fromScale(const std::string& key, double scaled)
{
  switch (scaleOf(key))
  {
    case ShareScale::Mired:  return (scaled > 0.0) ? 1000000.0 / scaled : 60000.0;
    case ShareScale::Factor: return std::exp(scaled);
    default:                 return scaled;
  }
}

// the gap of an image (its own part, never shared), kept by its tool in the file data of the server:
// given back with the key when the image lost its values (pp3 removed)
static const std::map<std::string, std::string> GAP_NAMESPACES = {
  {"Exposure/Compensation", "exposure"},
  {"White Balance/Temperature", "wb"},
};

// limits of the sliders: a shifted value is kept inside them
static const std::map<std::string, std::pair<double, double>> SHARE_LIMITS = {
  {"Exposure/Compensation", {-5.0, 12.0}},
  {"Exposure/Brightness", {-100.0, 100.0}},
  {"Exposure/Contrast", {-100.0, 100.0}},
  {"Exposure/Saturation", {-100.0, 100.0}},
  {"Exposure/Black", {-16384.0, 32768.0}},
  {"Exposure/HighlightCompr", {0.0, 500.0}},
  {"Exposure/HighlightComprThreshold", {0.0, 100.0}},
  {"Exposure/ShadowCompr", {0.0, 100.0}},
  {"Exposure/Clip", {0.0, 1.0}},
  {"White Balance/Temperature", {1500.0, 60000.0}},
  {"White Balance/Green", {0.02, 100.0}},
  {"White Balance/Equal", {0.5, 2.0}},
  {"Shadows & Highlights/Highlights", {0.0, 100.0}},
  {"Shadows & Highlights/HighlightTonalWidth", {10.0, 100.0}},
  {"Shadows & Highlights/Shadows", {0.0, 100.0}},
  {"Shadows & Highlights/ShadowTonalWidth", {10.0, 100.0}},
  {"Shadows & Highlights/Radius", {1.0, 100.0}},
};

// a shift as shown: in mireds for the temperature, as a factor for the tint and the equalizer
static Glib::ustring shiftText(const std::string& key, double shift)
{
  switch (scaleOf(key))
  {
    case ShareScale::Mired:  return Glib::ustring::format(std::showpos, std::fixed, std::setprecision(1), shift) + " mired";
    case ShareScale::Factor: return "x" + Glib::ustring::format(std::fixed, std::setprecision(3), std::exp(shift));
    default:                 return Glib::ustring::format(std::showpos, shift);
  }
}

// a shifted value, in the scale of the key, kept inside the limits of its slider
static double shifted(const std::string& key, double value, double shift)
{
  double scaled;
  if (!toScale(key, value, scaled))
    return value;
  value = fromScale(key, scaled + shift);
  auto limits = SHARE_LIMITS.find(key);
  if (limits != SHARE_LIMITS.end())
    value = std::max(limits->second.first, std::min(limits->second.second, value));
  return value;
}

// a value of the pp3 read as a number (the whole text); true/false and lists are not numbers
static bool parseNumber(const std::string& text, double& value)
{
  if (text.empty())
    return false;
  const char* begin = text.c_str();
  char* end = nullptr;
  value = g_ascii_strtod(begin, &end);
  return (end != begin) && (*end == '\0') && std::isfinite(value);
}

// keys whose value is a real number: "0" in a pp3 is not an integer for them (a shift of 0.5 received by
// a compensation of 0 was rounded to 0 or 1)
static const std::set<std::string> REAL_KEYS = {
  "Exposure/Compensation", "Exposure/Clip", "White Balance/Green", "White Balance/Equal",
};

static bool isInteger(const std::string& key, const std::string& text)
{
  if (REAL_KEYS.count(key) > 0)
    return false;
  return text.find_first_of(".eE") == std::string::npos;
}

TTShare::TTShare() : FoldableToolPanel(this, "ttshare", M("TT_SHARE_LABEL"), false, true)
{
  hasSnapshot = false;
  stateLoaded = false;
  seriesCount = parentCount = -1;
  generation = 0;
  connected = false;
  pending = false;
  focusConnected = false;

  cbShare = Gtk::manage(new Gtk::CheckButton(M("TT_SHARE_CHANGES")));
  cbShare->set_tooltip_text(M("TT_SHARE_CHANGES_TOOLTIP"));
  cbShare->set_active(esbySettings().ShareChanges);
  cbShare->signal_toggled().connect([this]() { esbySettings().ShareChanges = cbShare->get_active(); });
  pack_start(*cbShare, Gtk::PACK_SHRINK, 0);

  Gtk::Label* lbSections = Gtk::manage(new Gtk::Label(M("TT_SHARE_SECTIONS")));
  lbSections->set_xalign(0.0);
  pack_start(*lbSections, Gtk::PACK_SHRINK, 0);
  std::set<std::string> active = sharedSections();
  for (const auto& section : SHARE_SECTIONS)
  {
    Gtk::CheckButton* cb = Gtk::manage(new Gtk::CheckButton(M(section.second)));
    cb->set_active(active.count(section.first) > 0);
    cb->signal_toggled().connect([this]() {
      Glib::ustring list;
      for (const auto& s : SHARE_SECTIONS)
        if (cbSections[s.first]->get_active())
          list += (list.empty() ? "" : ";") + s.first;
      esbySettings().ShareSections = list;
    });
    cbSections[section.first] = cb;
    pack_start(*cb, Gtk::PACK_SHRINK, 0);
  }

  // parent folder: it initializes the series used for the first time
  Gtk::FlowBox* parentBox = Gtk::manage(new Gtk::FlowBox());
  parentBox->set_selection_mode(Gtk::SELECTION_NONE);
  parentBox->set_column_spacing(4);
  parentBox->set_row_spacing(2);
  btToParent = Gtk::manage(new Gtk::Button(M("TT_SHARE_TO_PARENT")));
  btToParent->set_tooltip_text(M("TT_SHARE_TO_PARENT_TOOLTIP"));
  btFromParent = Gtk::manage(new Gtk::Button(M("TT_SHARE_FROM_PARENT")));
  btFromParent->set_tooltip_text(M("TT_SHARE_FROM_PARENT_TOOLTIP"));
  parentBox->add(*btToParent);
  parentBox->add(*btFromParent);
  pack_start(*parentBox, Gtk::PACK_SHRINK, 0);
  btToParent->signal_clicked().connect([this]() { copyWithParent(true); });
  btFromParent->signal_clicked().connect([this]() { copyWithParent(false); });

  lbInfo = Gtk::manage(new Gtk::Label(""));
  lbInfo->set_line_wrap(true);
  lbInfo->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
  lbInfo->set_xalign(0.0);
  lbInfo->set_tooltip_text(M("TT_SHARE_COPY_TOOLTIP"));
  // a click on the status copies it to the clipboard
  Gtk::EventBox* infoBox = Gtk::manage(new Gtk::EventBox());
  infoBox->add(*lbInfo);
  infoBox->signal_button_press_event().connect([this](GdkEventButton*) {
    Gtk::Clipboard::get()->set_text(lbInfo->get_text());
    return true;
  });
  // small button with an icon, on the left: a labelled button took the whole width
  Gtk::Button* btCopy = Gtk::manage(new Gtk::Button());
  btCopy->set_image(*Gtk::manage(new RTImage("copy", Gtk::ICON_SIZE_MENU)));
  btCopy->set_relief(Gtk::RELIEF_NONE);
  btCopy->set_halign(Gtk::ALIGN_START);
  btCopy->set_valign(Gtk::ALIGN_START);
  btCopy->set_tooltip_text(M("TT_SHARE_COPY_TOOLTIP"));
  btCopy->signal_clicked().connect([this]() { Gtk::Clipboard::get()->set_text(lbInfo->get_text()); });
  pack_start(*btCopy, Gtk::PACK_SHRINK, 0);
  pack_start(*infoBox, Gtk::PACK_SHRINK, 0);
}

TTShare::~TTShare()
{
  if ((env != nullptr) && connected)
  {
    paramsConnection.disconnect();
    env->sharedVariables()->getClient()->setShareEventCallback(nullptr);
  }
}

void TTShare::deploy()
{
  FoldableToolPanel::deploy();
}

void TTShare::deployLate()
{
}

std::set<std::string> TTShare::sharedSections()
{
  std::set<std::string> sections;
  std::stringstream list(esbySettings().ShareSections.raw());
  std::string section;
  while (std::getline(list, section, ';'))
    if (!section.empty())
      sections.insert(section);
  return sections;
}

bool TTShare::isShared(const std::string& key)
{
  std::set<std::string> sections = sharedSections();
  if (sections.count(key.substr(0, key.find('/'))) == 0)
    return false;
  // a key the image does not take part in (TTSeriesWB: photo without flash)
  return !env->shareKeyFilter || env->shareKeyFilter(key);
}

Glib::ustring TTShare::tempFile(const char* name)
{
  return Glib::build_filename(Glib::get_user_runtime_dir(),
                              Glib::ustring::compose("esby-share-%1-%2.pp3", getpid(), name));
}

// the keys of the shared sections, as written in a pp3 ("Section/Key" -> text)
std::map<std::string, std::string> TTShare::extract(const ProcParams& params)
{
  std::map<std::string, std::string> keys;
  ProcParams copy = params;
  Glib::ustring tmp = tempFile("read");
  if (copy.save(tmp) != 0)
    return keys;
  Glib::KeyFile kf;
  try
  {
    kf.load_from_file(tmp);
    for (const std::string& section : sharedSections())
    {
      if (!kf.has_group(section))
        continue;
      auto only = SHARE_KEYS.find(section);
      for (const Glib::ustring& key : kf.get_keys(section))
        if ((only == SHARE_KEYS.end()) || (only->second.count(key.raw()) > 0))
          keys[section + "/" + key.raw()] = kf.get_value(section, key).raw();
    }
    // "Camera" white balance: the engine applies the camera multipliers, the temperature and tint of
    // the pp3 may be the ones of a profile. The values of the camera are the values of the image: a
    // shift received is applied to them, and a change to "Custom" from them is not a shift
    cameraValues(keys, params.wb.observer);
  }
  catch (const Glib::Error& e)
  {
    TT_LOG("TTShare: %s\n", e.what().c_str());
  }
  g_unlink(tmp.c_str());
  return keys;
}

// "Camera" white balance: the values of the camera replace the ones of the pp3 (see extract)
void TTShare::cameraValues(std::map<std::string, std::string>& keys, rtengine::StandardObserver observer)
{
  auto method = keys.find("White Balance/Setting");
  WBProvider* provider = env->getWBProvider();
  if ((method == keys.end()) || (method->second != "Camera") || (provider == nullptr))
    return;
  double camTemp = -1.0, camGreen = -1.0;
  provider->getCamWB(camTemp, camGreen, observer);
  if ((camTemp <= 0.0) || (camGreen <= 0.0)) // no image data yet
    return;
  keys["White Balance/Temperature"] = std::to_string(std::lround(camTemp));
  keys["White Balance/Green"] = Glib::Ascii::dtostr(camGreen);
  keys["White Balance/Equal"] = "1";
}

void TTShare::connectSignals()
{
  if (connected || (env == nullptr))
    return;
  connected = true;
  paramsConnection = env->paramsChangedSignal.connect([this](const ProcParams& params, const ProcEvent& ev, bool fromPanel) {
    paramsChanged(params, ev, fromPanel);
  });
  // a change in the series, from another instance (or this one): what this image missed
  env->sharedVariables()->getClient()->setShareEventCallback([this](const Glib::ustring& path) {
    if (!stateLoaded || (path != folder) || !getExpander()->getEnabled())
      return;
    int current = generation;
    env->sharedVariables()->getClient()->shareGet(folder, [this, current](bool ok, const std::vector<EsbyShareEntry>& entries) {
      if (ok && (current == generation))
      {
        catchUp(entries, true);
        refreshCounts();
      }
    });
  });
  // connection to the server (again): the state of the image is loaded if it was not
  env->sharedVariables()->signalChanged().connect([this]() {
    if (!stateLoaded && !file.empty() && env->sharedVariables()->isConnected())
      loadState();
  });
}

// a change of the parameters: from a tool (fromPanel), it is shared if its keys are shared; in every
// case, the parameters become the reference of the next change
void TTShare::paramsChanged(const ProcParams& params, const ProcEvent& ev, bool fromPanel)
{
  if (!getExpander()->getEnabled())
  {
    hasSnapshot = false;
    return;
  }
  std::map<std::string, std::string> current = extract(params);
  bool share = fromPanel && hasSnapshot && (env->shareSuppressed == 0) && cbShare->get_active()
            && (ev != EvPhotoLoaded);
  if (share)
  {
    // the camera values were maybe not known yet when the reference was taken
    cameraValues(snapshot, params.wb.observer);
    std::vector<EsbyShareChange> changes;
    for (const auto& kv : current)
    {
      auto before = snapshot.find(kv.first);
      if ((before == snapshot.end()) || (before->second == kv.second))
        continue;
      EsbyShareChange change;
      change.key = kv.first;
      double a, b;
      if (!isShared(kv.first))
        continue;
      double sa, sb;
      if (parseNumber(before->second, a) && parseNumber(kv.second, b)
      && toScale(kv.first, a, sa) && toScale(kv.first, b, sb))
      {
        change.delta = sb - sa;
        if (std::fabs(change.delta) < 1e-9)
          continue;
      }
      else
      {
        change.numeric = false;
        change.value = kv.second;
      }
      changes.push_back(change);
    }
    if (!changes.empty())
    {
      if (!stateLoaded || !env->sharedVariables()->isConnected())
        setInfo(M("TT_SHARE_NOT_SENT"));
      else
      {
        // this image has its own shifts already: they are counted as received
        std::vector<std::string> numeric;
        for (const EsbyShareChange& c : changes)
          if (c.numeric)
          {
            appliedCum[c.key.raw()] += c.delta;
            numeric.push_back(c.key.raw());
          }
        saveApplied(numeric);
        int current = generation;
        env->sharedVariables()->getClient()->shareChange(folder, changes,
          [this, current](bool ok, const std::vector<EsbyShareEntry>& entries) {
            if (!ok || (current != generation))
              return;
            std::vector<std::string> values;
            for (const EsbyShareEntry& e : entries)
              if (!e.numeric)
              {
                appliedVersion[e.key.raw()] = e.version;
                values.push_back(e.key.raw());
              }
            saveApplied(values);
          });
        Glib::ustring text;
        for (const EsbyShareChange& c : changes)
          text += "\n" + c.key + (c.numeric ? ": " + shiftText(c.key.raw(), c.delta) : Glib::ustring());
        setInfo(M("TT_SHARE_SENT") + text);
        refreshCounts();
      }
    }
  }
  if (stateLoaded && (file == EsbySharedVariables::originalFile(env)))
    saveValues(current, false);
  snapshot = current;
  hasSnapshot = true;
}

// the values of the shared keys of the image, as last seen (s:key): an image whose values differ when it
// is opened again (pp3 removed, profile applied elsewhere) has lost what it had received for these keys
void TTShare::saveValues(const std::map<std::string, std::string>& values, bool all)
{
  std::vector<EsbyVarValue> fields;
  for (const auto& kv : values)
  {
    auto known = savedValues.find(kv.first);
    if (!all && (known != savedValues.end()) && (known->second == kv.second))
      continue;
    EsbyVarValue v;
    v.name = "s:" + kv.first;
    v.s = kv.second;
    fields.push_back(v);
    savedValues[kv.first] = kv.second;
  }
  if (!fields.empty() && !file.empty())
    env->sharedVariables()->getClient()->fileSet(file, "share", fields, nullptr);
}

// the state of the image opened: what it has already received, then what it missed
void TTShare::loadState()
{
  stateLoaded = false;
  int current = ++generation;
  seriesCount = parentCount = -1;
  file = EsbySharedVariables::originalFile(env);
  folder = file.empty() ? Glib::ustring() : EsbySharedVariables::seriesFolder(file);
  appliedCum.clear();
  appliedVersion.clear();
  pending = false;
  EsbyWBClient* client = env->sharedVariables()->getClient();
  if (folder.empty() || !client->isConnected())
  {
    setInfo(M("TT_SHARE_NO_SERVER"));
    return;
  }
  client->fileGet(file, "share", [this, current, client](bool ok, const std::vector<EsbyVarValue>& fields) {
    if (!ok || (current != generation))
      return;
    savedValues.clear();
    for (const EsbyVarValue& f : fields)
    {
      std::string name = f.name.raw();
      double number = (f.type == RT_VARIABLE_TYPE_INT) ? f.i : f.d;
      if (name.compare(0, 2, "c:") == 0)
        appliedCum[name.substr(2)] = number;
      else if (name.compare(0, 2, "v:") == 0)
        appliedVersion[name.substr(2)] = (int) number;
      else if (name.compare(0, 2, "s:") == 0)
        savedValues[name.substr(2)] = f.s.raw();
    }
    // a key whose value is not the one last seen (pp3 removed: values of the default profile, profile
    // applied elsewhere): what the image had received for it is lost, it receives everything again
    bool lost = false;
    std::vector<std::string> lostGaps; // keys lost whose tool keeps a gap of the image
    ProcParams params;
    EsbyParamsAccess* access = env->getParamsAccess();
    if ((access != nullptr) && access->esbyGetParams(params))
    {
      std::map<std::string, std::string> values = extract(params);
      // "Camera" white balance, now and when last seen: its values are the ones of the camera, maybe
      // not known yet (see extract), they are not compared
      auto method = values.find("White Balance/Setting");
      auto knownMethod = savedValues.find("White Balance/Setting");
      bool cameraBoth = (method != values.end()) && (method->second == "Camera")
                     && (knownMethod != savedValues.end()) && (knownMethod->second == "Camera");
      for (const auto& kv : values)
      {
        auto known = savedValues.find(kv.first);
        if (cameraBoth && (kv.first != "White Balance/Setting") && (kv.first.compare(0, 14, "White Balance/") == 0))
          continue;
        if ((known != savedValues.end()) && (known->second != kv.second))
        {
          appliedCum.erase(kv.first);
          appliedVersion.erase(kv.first);
          lost = true;
          if (GAP_NAMESPACES.count(kv.first))
            lostGaps.push_back(kv.first);
        }
      }
      saveValues(values, true);
    }
    stateLoaded = true;
    // a series used for the first time starts from its parent folder. A lost compensation (temperature)
    // also gets the gap of the image with the reference of its series again (TTSeriesExposure, TTSeriesWB)
    auto init = [this, current, client, lost](const std::map<std::string, double>& extra) {
      client->shareInit(folder, EsbySharedVariables::parentFolder(folder), [this, current, lost, extra](bool ok2, const std::vector<EsbyShareEntry>& entries, bool copied,
                                                             const Glib::ustring& origin) {
        if (!ok2 || (current != generation))
          return;
        Glib::ustring parent = origin.empty() ? Glib::ustring() : Glib::ustring(Glib::path_get_basename(origin));
        Glib::ustring message;
        if (copied)
          message = Glib::ustring::compose(M(entries.empty() ? "TT_SHARE_FROM_PARENT_INIT_EMPTY" : "TT_SHARE_FROM_PARENT_INIT"), parent);
        else if (lost && !parent.empty())
          message = Glib::ustring::compose(M("TT_SHARE_FRESH_PARENT"), parent);
        else if (lost)
          message = M("TT_SHARE_FRESH");
        catchUp(entries, false, message, extra);
        if (copied && entries.empty())
          setInfo(message); // nothing applied: the message is shown anyway
        refreshCounts();
      });
    };
    // the gaps are read one after the other, then the series is applied
    auto extra = std::make_shared<std::map<std::string, double>>();
    auto next = std::make_shared<std::function<void(size_t)>>();
    std::weak_ptr<std::function<void(size_t)>> weak = next; // a pending request holds it, not itself
    *next = [this, current, client, init, lostGaps, extra, weak](size_t i) {
      if (i >= lostGaps.size())
      {
        init(*extra);
        return;
      }
      std::string key = lostGaps[i];
      std::shared_ptr<std::function<void(size_t)>> self = weak.lock();
      client->fileGet(file, GAP_NAMESPACES.at(key), [this, current, key, extra, self, i](bool ok3, const std::vector<EsbyVarValue>& data) {
        if (current != generation)
          return;
        for (const EsbyVarValue& f : data)
          if (ok3 && (f.name == "gap"))
            (*extra)[key] = (f.type == RT_VARIABLE_TYPE_INT) ? f.i : f.d;
        (*self)(i + 1);
      });
    };
    (*next)(0);
  });
}

// what the image has not received yet: the shifts (sum of the series - sum received) and the values of
// a newer version. fromEvent: a change of another image; applied in the active window only (N instances
// do not recalculate together), the others do it when they get the focus
void TTShare::catchUp(const std::vector<EsbyShareEntry>& entries, bool fromEvent, const Glib::ustring& origin,
                      std::map<std::string, double> extra)
{
  EsbyParamsAccess* access = env->getParamsAccess();
  if ((access == nullptr) || !stateLoaded)
    return;

  Gtk::Window* window = dynamic_cast<Gtk::Window*>(get_toplevel());
  if ((window != nullptr) && !focusConnected)
  {
    // the window becomes active in the default handler of focus-in: the changes are applied just after
    window->signal_focus_in_event().connect([this](GdkEventFocus*) {
      if (pending)
        Glib::signal_idle().connect_once([this]() {
          if (!pending || !stateLoaded)
            return;
          int current = generation;
          env->sharedVariables()->getClient()->shareGet(folder, [this, current](bool ok, const std::vector<EsbyShareEntry>& e) {
            if (ok && (current == generation))
              catchUp(e, false);
          });
        });
      return false;
    }, true);
    focusConnected = true;
  }

  ProcParams params;
  if (!access->esbyGetParams(params))
    return;
  std::map<std::string, std::string> current = extract(params);
  // computed on copies: nothing is counted as received while it waits for the focus
  std::map<std::string, double> cum = appliedCum;
  std::map<std::string, int> version = appliedVersion;
  Glib::KeyFile changes;
  std::vector<std::string> received;
  Glib::ustring text;
  bool exposureOnly = true;
  int count = 0;
  for (const EsbyShareEntry& e : entries)
  {
    std::string key = e.key.raw();
    if (!isShared(key))
      continue;
    auto cur = current.find(key);
    std::string section = key.substr(0, key.find('/'));
    std::string name = key.substr(key.find('/') + 1);
    if (e.numeric)
    {
      double missing = e.cum - (cum.count(key) ? cum[key] : 0.0);
      double value;
      if (extra.count(key))
      {
        missing += extra[key]; // the gap of the image, lost with its values
        extra.erase(key);
      }
      if (std::fabs(missing) < 1e-9)
        continue;
      if ((cur != current.end()) && parseNumber(cur->second, value))
      {
        value = shifted(key, value, missing);
        changes.set_value(section, name, isInteger(key, cur->second) ? std::to_string(std::lround(value))
                                                                      : Glib::Ascii::dtostr(value));
        text += "\n" + key + ": " + shiftText(key, missing);
        count++;
      }
      cum[key] = e.cum;
    }
    else
    {
      if (e.version <= (version.count(key) ? version[key] : 0))
        continue;
      if ((cur == current.end()) || (cur->second != e.value.raw()))
      {
        changes.set_value(section, name, e.value);
        text += "\n" + key;
        count++;
      }
      version[key] = e.version;
    }
    if (section != "Exposure")
      exposureOnly = false;
    received.push_back(key);
  }

  // a gap of the image for a key the series does not share
  for (const auto& x : extra)
  {
    auto cur = current.find(x.first);
    double value;
    if ((std::fabs(x.second) < 1e-9) || (cur == current.end()) || !parseNumber(cur->second, value))
      continue;
    value = shifted(x.first, value, x.second);
    changes.set_value(x.first.substr(0, x.first.find('/')), x.first.substr(x.first.find('/') + 1),
                      isInteger(x.first, cur->second) ? std::to_string(std::lround(value)) : Glib::Ascii::dtostr(value));
    text += "\n" + x.first + ": " + shiftText(x.first, x.second);
    if (x.first.compare(0, 14, "White Balance/") == 0)
      exposureOnly = false;
    count++;
  }

  // a white balance changed while the image has the "Camera" one: it becomes "Custom", from the values
  // of the camera (see extract), and a change of the method alone does not keep values of a profile
  auto method = current.find("White Balance/Setting");
  if ((count > 0) && changes.has_group("White Balance") && (method != current.end()) && (method->second == "Camera"))
  {
    if (!changes.has_key("White Balance", "Setting"))
      changes.set_value("White Balance", "Setting", "Custom");
    if (changes.get_value("White Balance", "Setting") == "Custom")
      for (const char* k : {"Temperature", "Green", "Equal"})
        if (!changes.has_key("White Balance", k) && current.count(std::string("White Balance/") + k))
          changes.set_value("White Balance", k, current[std::string("White Balance/") + k]);
  }

  if ((count > 0) && fromEvent && (window != nullptr) && !window->is_active())
  {
    pending = true;
    setInfo(M("TT_SHARE_PENDING"));
    return;
  }
  pending = false;
  appliedCum = cum;
  appliedVersion = version;
  if (count > 0)
  {
    changes.set_integer("Version", "Version", PPVERSION);
    Glib::ustring tmp = tempFile("apply");
    try
    {
      Glib::file_set_contents(tmp, changes.to_data());
      params.load(tmp);
    }
    catch (const Glib::Error& e)
    {
      TT_LOG("TTShare: %s\n", e.what().c_str());
    }
    g_unlink(tmp.c_str());
    env->shareSuppressed++;
    access->esbyApplyParams(params, exposureOnly ? ProcEvent(EvExpComp) : ProcEvent(EvProfileChanged), M("TT_SHARE_HISTORY"));
    env->shareSuppressed--;
    setInfo((origin.empty() ? M("TT_SHARE_RECEIVED") : origin) + text);
  }
  else if (!fromEvent)
    setInfo("");
  saveApplied(received);
}

// what the image has received, kept by the server (c:key sum of the shifts, v:key version)
void TTShare::saveApplied(const std::vector<std::string>& keys)
{
  if (keys.empty() || file.empty())
    return;
  std::vector<EsbyVarValue> fields;
  for (const std::string& key : keys)
  {
    EsbyVarValue v;
    if (appliedVersion.count(key) && !appliedCum.count(key))
    {
      v.name = "v:" + key;
      v.type = RT_VARIABLE_TYPE_INT;
      v.i = appliedVersion[key];
    }
    else
    {
      v.name = "c:" + key;
      v.type = RT_VARIABLE_TYPE_DOUBLE;
      v.d = appliedCum[key];
    }
    fields.push_back(v);
  }
  env->sharedVariables()->getClient()->fileSet(file, "share", fields, nullptr);
}

// toParent: the shared parameters of the series replace the ones of the parent folder (the series used
// for the first time afterwards start from them); otherwise the series takes again the ones of the parent
// (its images are changed), after a confirmation
void TTShare::copyWithParent(bool toParent)
{
  if (!stateLoaded || folder.empty() || !env->sharedVariables()->isConnected())
  {
    setInfo(M("TT_SHARE_NO_SERVER"));
    return;
  }
  Glib::ustring parent = EsbySharedVariables::parentFolder(folder);
  if (!toParent)
  {
    Gtk::Window* window = dynamic_cast<Gtk::Window*>(get_toplevel());
    Gtk::MessageDialog dialog(Glib::ustring::compose(M("TT_SHARE_FROM_PARENT_CONFIRM"), Glib::path_get_basename(folder),
                                                     Glib::path_get_basename(parent)),
                              false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL, true);
    if (window != nullptr)
      dialog.set_transient_for(*window);
    if (dialog.run() != Gtk::RESPONSE_OK)
      return;
  }
  Glib::ustring from = toParent ? folder : parent;
  Glib::ustring to = toParent ? parent : folder;
  env->sharedVariables()->getClient()->shareCopy(from, to, [this, toParent, parent](bool ok, const std::vector<EsbyShareEntry>& keys) {
    // the series changed: the event of the server brings the changes to its images
    Glib::ustring name = Glib::path_get_basename(parent);
    if (!ok)
      setInfo(M("TT_SHARE_NO_SERVER"));
    else if (toParent)
    {
      // an empty series empties the parent: the next series start without shared change
      int count = 0;
      for (const EsbyShareEntry& k : keys)
        if (!k.numeric || (std::fabs(k.cum) >= 1e-9))
          count++;
      setInfo(count == 0 ? Glib::ustring::compose(M("TT_SHARE_PARENT_EMPTIED"), name)
                         : Glib::ustring::compose(M("TT_SHARE_SENT_TO_PARENT"), name, count));
    }
    else
      setInfo(Glib::ustring::compose(M("TT_SHARE_RESET_FROM_PARENT"), name));
    refreshCounts();
  });
}

// the number of shared changes of the series and of its parent, shown in the status
void TTShare::refreshCounts()
{
  EsbyWBClient* client = env->sharedVariables()->getClient();
  if (folder.empty() || !client->isConnected())
    return;
  int current = generation;
  client->shareGet(folder, [this, current](bool ok, const std::vector<EsbyShareEntry>& e) {
    if (ok && (current == generation))
    {
      seriesCount = (int) e.size();
      setInfo(lastText);
    }
  });
  client->shareGet(EsbySharedVariables::parentFolder(folder), [this, current](bool ok, const std::vector<EsbyShareEntry>& e) {
    if (ok && (current == generation))
    {
      parentCount = (int) e.size();
      setInfo(lastText);
    }
  });
}

void TTShare::setInfo(const Glib::ustring& text)
{
  lastText = text;
  Glib::ustring status;
  if (folder.empty())
    status = Glib::ustring::compose(M("TT_SHARE_SERIES"), "-");
  else
  {
    Glib::ustring series = Glib::path_get_basename(folder);
    Glib::ustring parent = Glib::path_get_basename(EsbySharedVariables::parentFolder(folder));
    status = (seriesCount < 0) ? Glib::ustring::compose(M("TT_SHARE_SERIES"), series)
           : Glib::ustring::compose(M("TT_SHARE_SERIES_COUNT"), series, seriesCount);
    status += "\n" + ((parentCount < 0) ? Glib::ustring::compose(M("TT_SHARE_PARENT"), parent)
                     : Glib::ustring::compose(M("TT_SHARE_PARENT_COUNT"), parent, parentCount));
  }
  lbInfo->set_text(status + (text.empty() ? "" : "\n" + text));
  TT_LOG("TTShare: %s\n", text.c_str());
}

// enabled: the current image is followed from now on
void TTShare::enabledChanged()
{
  if (!getExpander()->getEnabled() || (env == nullptr))
    return;
  connectSignals();
  ProcParams params;
  EsbyParamsAccess* access = env->getParamsAccess();
  if ((access != nullptr) && access->esbyGetParams(params))
  {
    snapshot = extract(params);
    hasSnapshot = true;
  }
  if (!EsbySharedVariables::originalFile(env).empty())
    loadState();
}

void TTShare::react(FakeProcEvent ev)
{
  connectSignals();
  if ((ev == FakeEvExifTransmitted) && getExpander()->getEnabled())
    loadState();
}

Glib::ustring TTShare::themeExport()
{
  return getToolName() + ":" + "active " + std::string(getExpander()->getEnabled() ? "1" : "0") + "\n";
}

void TTShare::themeImport(std::ifstream& myfile)
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
