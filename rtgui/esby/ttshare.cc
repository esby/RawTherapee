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
#include "rtengine/procparams.h"
#include "rtengine/procevents.h"
#include <cmath>
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
};

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

static bool isInteger(const std::string& text)
{
  return text.find_first_of(".eE") == std::string::npos;
}

TTShare::TTShare() : FoldableToolPanel(this, "ttshare", M("TT_SHARE_LABEL"), false, true)
{
  hasSnapshot = false;
  stateLoaded = false;
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
  Gtk::Button* btCopy = Gtk::manage(new Gtk::Button(M("TT_SHARE_COPY")));
  btCopy->set_image(*Gtk::manage(new RTImage("copy")));
  btCopy->set_always_show_image(true);
  btCopy->set_tooltip_text(M("TT_SHARE_COPY_TOOLTIP"));
  btCopy->signal_clicked().connect([this]() { Gtk::Clipboard::get()->set_text(lbInfo->get_text()); });
  pack_start(*btCopy, Gtk::PACK_SHRINK, 0);
  pack_start(*infoBox, Gtk::PACK_SHRINK, 0);
}

TTShare::~TTShare()
{
  if ((env != nullptr) && connected)
  {
    env->paramsListener = nullptr;
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
  return sections.count(key.substr(0, key.find('/'))) > 0;
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
      for (const Glib::ustring& key : kf.get_keys(section))
        keys[section + "/" + key.raw()] = kf.get_value(section, key).raw();
    }
  }
  catch (const Glib::Error& e)
  {
    TT_LOG("TTShare: %s\n", e.what().c_str());
  }
  g_unlink(tmp.c_str());
  return keys;
}

void TTShare::connectSignals()
{
  if (connected || (env == nullptr))
    return;
  connected = true;
  env->paramsListener = [this](const ProcParams& params, const ProcEvent& ev, bool fromPanel) {
    paramsChanged(params, ev, fromPanel);
  };
  // a change in the series, from another instance (or this one): what this image missed
  env->sharedVariables()->getClient()->setShareEventCallback([this](const Glib::ustring& path) {
    if (!stateLoaded || (path != folder) || !getExpander()->getEnabled())
      return;
    int current = generation;
    env->sharedVariables()->getClient()->shareGet(folder, [this, current](bool ok, const std::vector<EsbyShareEntry>& entries) {
      if (ok && (current == generation))
        catchUp(entries, true);
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
    std::vector<EsbyShareChange> changes;
    for (const auto& kv : current)
    {
      auto before = snapshot.find(kv.first);
      if ((before == snapshot.end()) || (before->second == kv.second))
        continue;
      EsbyShareChange change;
      change.key = kv.first;
      double a, b;
      if (parseNumber(before->second, a) && parseNumber(kv.second, b))
      {
        change.delta = b - a;
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
          text += "\n" + c.key + (c.numeric ? ": " + Glib::ustring::format(std::showpos, c.delta) : Glib::ustring());
        setInfo(M("TT_SHARE_SENT") + text);
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
  file = EsbySharedVariables::originalFile(env);
  folder = file.empty() ? Glib::ustring() : EsbySharedVariables::realPath(EsbySharedVariables::sequenceFolder(file));
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
    ProcParams params;
    EsbyParamsAccess* access = env->getParamsAccess();
    if ((access != nullptr) && access->esbyGetParams(params))
    {
      std::map<std::string, std::string> values = extract(params);
      for (const auto& kv : values)
      {
        auto known = savedValues.find(kv.first);
        if ((known != savedValues.end()) && (known->second != kv.second))
        {
          appliedCum.erase(kv.first);
          appliedVersion.erase(kv.first);
          lost = true;
        }
      }
      saveValues(values, true);
    }
    stateLoaded = true;
    // a series used for the first time starts from its parent folder
    client->shareInit(folder, [this, current, lost](bool ok2, const std::vector<EsbyShareEntry>& entries, bool copied) {
      if (ok2 && (current == generation))
        catchUp(entries, false, copied ? M("TT_SHARE_FROM_PARENT_INIT") : lost ? M("TT_SHARE_FRESH") : Glib::ustring());
    });
  });
}

// what the image has not received yet: the shifts (sum of the series - sum received) and the values of
// a newer version. fromEvent: a change of another image; applied in the active window only (N instances
// do not recalculate together), the others do it when they get the focus
void TTShare::catchUp(const std::vector<EsbyShareEntry>& entries, bool fromEvent, const Glib::ustring& origin)
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
      if (std::fabs(missing) < 1e-9)
        continue;
      if ((cur != current.end()) && parseNumber(cur->second, value))
      {
        value += missing;
        auto limits = SHARE_LIMITS.find(key);
        if (limits != SHARE_LIMITS.end())
          value = std::max(limits->second.first, std::min(limits->second.second, value));
        changes.set_value(section, name, isInteger(cur->second) ? std::to_string(std::lround(value))
                                                                 : Glib::Ascii::dtostr(value));
        text += "\n" + key + ": " + Glib::ustring::format(std::showpos, missing);
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
  Glib::ustring parent = Glib::path_get_dirname(folder);
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
  env->sharedVariables()->getClient()->shareCopy(from, to, [this, toParent](bool ok, const std::vector<EsbyShareEntry>&) {
    // the series changed: the event of the server brings the changes to its images
    setInfo(M(!ok ? "TT_SHARE_NO_SERVER" : toParent ? "TT_SHARE_SENT_TO_PARENT" : "TT_SHARE_RESET_FROM_PARENT"));
  });
}

void TTShare::setInfo(const Glib::ustring& text)
{
  Glib::ustring series = folder.empty() ? Glib::ustring("-") : Glib::ustring(Glib::path_get_basename(folder));
  lbInfo->set_text(Glib::ustring::compose(M("TT_SHARE_SERIES"), series) + (text.empty() ? "" : "\n" + text));
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
