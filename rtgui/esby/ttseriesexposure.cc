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
#include "tools/tonecurve.h"
#include "rtengine/color.h"
#include "rtengine/procparams.h"
#include <exiv2/exiv2.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>

using namespace rtengine;
using namespace rtengine::procparams;

// limits of the exposure compensation adjuster (tools/tonecurve.cc)
static const double SERIESEXP_MIN_COMP = -5.0;
static const double SERIESEXP_MAX_COMP = 12.0;

TTSeriesExposure::TTSeriesExposure() :
  FoldableToolPanel(this, "TTSeriesExposure", M("TT_SERIESEXP_LABEL"), false, true),
  EditSubscriber(ET_PIPETTE)
{
  tonecurve = nullptr;
  sequenceReady = false;

  // the pipette of the first tone curve of the exposure tool: values after the exposure
  // compensation, the brightness, the contrast and the highlight compression, before the curves
  setEditID(EUID_ToneCurve1, BT_IMAGEFLOAT);

  tbPipette = Gtk::manage(new Gtk::ToggleButton(M("TT_SERIESEXP_PIPETTE")));
  tbPipette->set_tooltip_text(M("TT_SERIESEXP_PIPETTE_TOOLTIP"));
  pack_start(*tbPipette, Gtk::PACK_SHRINK, 0);

  adjGap = Gtk::manage(new Adjuster(M("TT_SERIESEXP_GAP"), 1.0, 30.0, 0.5, 3.0));
  adjGap->setAdjusterListener(this);
  adjGap->set_tooltip_text(M("TT_SERIESEXP_GAP_TOOLTIP"));
  adjGap->block(true);
  adjGap->setValue(esbySettings().SeriesExpGap);
  adjGap->block(false);
  pack_start(*adjGap, Gtk::PACK_SHRINK, 0);

  lbInfo = Gtk::manage(new Gtk::Label(""));
  lbInfo->set_line_wrap(true);
  lbInfo->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
  lbInfo->set_xalign(0.0);
  pack_start(*lbInfo, Gtk::PACK_SHRINK, 0);

  tbPipette->signal_toggled().connect([this]() {
    if (tbPipette->get_active())
    {
      if (getEditProvider() != nullptr)
        subscribe();
    }
    else if (isCurrentSubscriber())
      unsubscribe();
  });
  scanDone.connect(sigc::mem_fun(*this, &TTSeriesExposure::sequenceScanned));
}

TTSeriesExposure::~TTSeriesExposure()
{
  // a scan still running must not use this object anymore
  if (scan)
  {
    std::lock_guard<std::mutex> lock(scan->mutex);
    scan->alive = false;
  }
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
// sequence
// ---------------------------------------------------------------------------------------

// ESBY_ORIGIN (see TTSeriesWB): the original file of a copy opened by a launcher such as rt_queue
Glib::ustring TTSeriesExposure::originalFile()
{
  if (env == nullptr)
    return "";
  Glib::ustring file = env->getVarAsString("Fname");
  const char* origin = g_getenv("ESBY_ORIGIN");
  if ((origin != nullptr) && (*origin != '\0') && !file.empty()
  && (Glib::path_get_basename(origin) == Glib::path_get_basename(file)))
    return origin;
  return file;
}

// the numbered folder of a photo: the raw files are in <folder>/pp, then <folder>/pp/dpp once processed
Glib::ustring TTSeriesExposure::sequenceFolder(const Glib::ustring& file)
{
  std::string dir = Glib::path_get_dirname(file);
  if ((Glib::path_get_basename(dir) == "dpp") && (Glib::path_get_basename(Glib::path_get_dirname(dir)) == "pp"))
    return Glib::path_get_dirname(Glib::path_get_dirname(dir));
  if (Glib::path_get_basename(dir) == "pp")
    return Glib::path_get_dirname(dir);
  return dir;
}

// number of models of a sequence: the "credit_cosplayer: model: ..." lines of its fields.conf, or,
// without fields.conf, the names of the credit field of a convention folder ("NNN - day - credits - ...").
// -1: unknown.
int TTSeriesExposure::countModels(const Glib::ustring& folder)
{
  std::ifstream fields(Glib::build_filename(folder, "fields.conf"));
  if (fields)
  {
    int count = 0;
    std::string line;
    while (std::getline(fields, line))
    {
      std::string::size_type start = line.find_first_not_of(" \t");
      if ((start == std::string::npos) || (line.compare(start, 16, "credit_cosplayer") != 0))
        continue;
      std::string::size_type colon = line.find(':', start);
      if (colon == std::string::npos)
        continue;
      std::string value = line.substr(colon + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      std::transform(value.begin(), value.end(), value.begin(), ::tolower);
      if (value.compare(0, 6, "model:") == 0)
        count++;
    }
    return count;
  }

  std::string name = Glib::path_get_basename(folder);
  // an empty field ("011 - samedi - - ada wong") gives " - - ": a space is added, so that the
  // split below keeps an empty field instead of a "- ada wong" one
  std::string::size_type empty;
  while ((empty = name.find(" - - ")) != std::string::npos)
    name.insert(empty + 3, " ");
  std::vector<std::string> fieldsOfName;
  std::string::size_type pos = 0, next;
  while ((next = name.find(" - ", pos)) != std::string::npos)
  {
    fieldsOfName.push_back(name.substr(pos, next - pos));
    pos = next + 3;
  }
  fieldsOfName.push_back(name.substr(pos));
  if (fieldsOfName.size() < 3)
    return -1;
  std::istringstream credits(fieldsOfName[2]);
  std::string credit;
  int count = 0;
  while (credits >> credit)
    count++;
  return (count > 0) ? count : -1;
}

// capture time of a raw file (seconds), -1 if unknown
double TTSeriesExposure::captureTime(const Glib::ustring& file)
{
  try
  {
    auto image = Exiv2::ImageFactory::open(std::string(file));
    image->readMetadata();
    Exiv2::ExifData& exif = image->exifData();
    auto date = exif.findKey(Exiv2::ExifKey("Exif.Photo.DateTimeOriginal"));
    if (date == exif.end())
      return -1.0;
    int y, mo, d, h, mi, s;
    if (sscanf(date->toString().c_str(), "%d:%d:%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s) != 6)
      return -1.0;
    double t = Glib::DateTime::create_utc(y, mo, d, h, mi, s).to_unix();
    auto subsec = exif.findKey(Exiv2::ExifKey("Exif.Photo.SubSecTimeOriginal"));
    if (subsec != exif.end())
      t += atof(("0." + subsec->toString()).c_str());
    return t;
  }
  catch (const std::exception&)
  {
    return -1.0;
  }
}

void TTSeriesExposure::startSequence()
{
  sequenceReady = false;
  Glib::ustring file = originalFile();
  if (file.empty())
  {
    setInfo(M("TT_SERIESEXP_NO_IMAGE"));
    return;
  }
  Glib::ustring folder = sequenceFolder(file);
  int models = countModels(folder);
  if (models != 1)
  {
    // several models, or unknown: the whole folder is the sequence
    sequenceKey = folder;
    sequenceText = (models > 1)
      ? Glib::ustring::compose(M("TT_SERIESEXP_SEQ_FOLDER"), Glib::path_get_basename(folder), models)
      : Glib::ustring::compose(M("TT_SERIESEXP_SEQ_UNKNOWN"), Glib::path_get_basename(folder));
    sequenceReady = true;
    setInfo("");
    return;
  }

  // one model: the capture times of the folder split it into parts
  if (scan && (scan->folder == folder))
  {
    bool done;
    {
      std::lock_guard<std::mutex> lock(scan->mutex);
      done = scan->done;
    }
    if (done)
      sequenceScanned();
    return; // otherwise sequenceScanned() is called when the scan ends
  }

  if (scan)
  {
    std::lock_guard<std::mutex> lock(scan->mutex);
    scan->alive = false; // a former scan, for another folder
  }
  scan = std::make_shared<Scan>();
  scan->folder = folder;
  setInfo(M("TT_SERIESEXP_SEQ_WAIT"));

  std::shared_ptr<Scan> s = scan;
  Glib::Dispatcher* done = &scanDone;
  std::thread([s, done, folder]() {
    static const std::vector<std::string> extensions = {".rw2", ".dng", ".cr2", ".cr3", ".nef", ".arw", ".orf", ".raf", ".pef"};
    std::map<Glib::ustring, double> times;
    for (const std::string& sub : {std::string(), std::string("pp"), std::string("pp/dpp")})
    {
      std::string dir = sub.empty() ? std::string(folder) : Glib::build_filename(std::string(folder), sub);
      try
      {
        Glib::Dir entries(dir);
        for (const std::string& name : entries)
        {
          std::string lower = name;
          std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
          std::string::size_type dot = lower.rfind('.');
          if ((dot == std::string::npos)
          || (std::find(extensions.begin(), extensions.end(), lower.substr(dot)) == extensions.end()))
            continue;
          double t = captureTime(Glib::build_filename(dir, name));
          if (t >= 0.0)
            times[name] = t;
        }
      }
      catch (const Glib::Error&) {} // no such sub-folder
    }
    std::lock_guard<std::mutex> lock(s->mutex);
    s->times = times;
    s->done = true;
    if (s->alive)
      done->emit();
  }).detach();
}

// the scan of the folder is done: the part of the current image
void TTSeriesExposure::sequenceScanned()
{
  if (!scan)
    return;
  std::map<Glib::ustring, double> times;
  Glib::ustring folder;
  {
    std::lock_guard<std::mutex> lock(scan->mutex);
    if (!scan->done)
      return;
    times = scan->times;
    folder = scan->folder;
  }
  Glib::ustring file = originalFile();
  if (file.empty() || (sequenceFolder(file) != folder))
    return; // another image was opened meanwhile

  double current = -1.0;
  auto it = times.find(Glib::path_get_basename(file));
  if (it != times.end())
    current = it->second;
  else
  {
    current = captureTime(env->getVarAsString("Fname")); // the opened copy
    if (current >= 0.0)
      times[Glib::path_get_basename(file)] = current;
  }

  std::vector<double> sorted;
  for (const auto& t : times)
    sorted.push_back(t.second);
  std::sort(sorted.begin(), sorted.end());
  if (sorted.empty() || (current < 0.0))
  {
    sequenceKey = folder;
    sequenceText = Glib::ustring::compose(M("TT_SERIESEXP_SEQ_UNKNOWN"), Glib::path_get_basename(folder));
    sequenceReady = true;
    setInfo("");
    return;
  }

  double gap = adjGap->getValue() * 60.0;
  std::vector<double> starts = {sorted[0]};
  for (size_t i = 1; i < sorted.size(); i++)
  {
    if (sorted[i] - sorted[i - 1] > gap)
      starts.push_back(sorted[i]);
  }
  size_t part = 0;
  for (size_t i = 0; i < starts.size(); i++)
  {
    if (starts[i] <= current + 0.5)
      part = i;
  }
  Glib::ustring start = Glib::DateTime::create_now_utc((gint64) starts[part]).format("%Y-%m-%d %H:%M:%S");
  sequenceKey = folder + "#" + start;
  sequenceText = Glib::ustring::compose(M("TT_SERIESEXP_SEQ_PART"), Glib::path_get_basename(folder),
                                        (int) part + 1, (int) starts.size(), start.substr(11));
  sequenceReady = true;
  setInfo("");
}

// ---------------------------------------------------------------------------------------
// measure and exposure
// ---------------------------------------------------------------------------------------

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

bool TTSeriesExposure::button1Pressed(int modifierKey)
{
  if (!getExpander()->getEnabled() || (tonecurve == nullptr))
    return false;
  if (!sequenceReady)
  {
    setInfo(M("TT_SERIESEXP_SEQ_WAIT"));
    return true;
  }
  double luminance;
  if (!measure(luminance))
  {
    setInfo(M("TT_SERIESEXP_NO_VALUE"));
    return true;
  }

  // ctrl + click: the brightness of the reference image becomes the target of the sequence
  if (modifierKey & GDK_CONTROL_MASK)
  {
    writeTarget(luminance, Glib::path_get_basename(originalFile()));
    setInfo(Glib::ustring::compose(M("TT_SERIESEXP_TARGET_SET"), Glib::ustring::format(std::fixed, std::setprecision(4), luminance)));
    return true;
  }

  double target;
  Glib::ustring reference;
  if (!readTarget(target, reference))
  {
    setInfo(M("TT_SERIESEXP_NO_TARGET"));
    return true;
  }
  // linear values: the gap in EV is log2 of their ratio. The tone curve of the exposure tool
  // (contrast, highlight compression) makes it approximate: a second click refines it.
  double ev = std::log2(target / luminance);
  double comp = std::max(SERIESEXP_MIN_COMP, std::min(SERIESEXP_MAX_COMP, currentExpComp() + ev));
  setExpComp(comp);
  setInfo(Glib::ustring::compose(M("TT_SERIESEXP_APPLIED"),
                                 Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), ev),
                                 Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), comp)));
  return true;
}

// ---------------------------------------------------------------------------------------
// targets, kept in a local file: one RawTherapee instance per image with rt_queue
// ---------------------------------------------------------------------------------------

Glib::ustring TTSeriesExposure::targetsFile()
{
  return Glib::build_filename(Options::rtdir, "esby-exposure-targets.ini");
}

bool TTSeriesExposure::readTarget(double& luminance, Glib::ustring& reference)
{
  if (sequenceKey.empty())
    return false;
  Glib::KeyFile keys;
  try
  {
    if (!Glib::file_test(targetsFile(), Glib::FILE_TEST_EXISTS) || !keys.load_from_file(targetsFile()))
      return false;
    if (!keys.has_group(sequenceKey) || !keys.has_key(sequenceKey, "target"))
      return false;
    luminance = keys.get_double(sequenceKey, "target");
    reference = keys.has_key(sequenceKey, "reference") ? keys.get_string(sequenceKey, "reference") : Glib::ustring();
    return luminance > 0.0;
  }
  catch (const Glib::Error&)
  {
    return false;
  }
}

void TTSeriesExposure::writeTarget(double luminance, const Glib::ustring& reference)
{
  if (sequenceKey.empty())
    return;
  Glib::KeyFile keys;
  try
  {
    if (Glib::file_test(targetsFile(), Glib::FILE_TEST_EXISTS))
      keys.load_from_file(targetsFile(), Glib::KEY_FILE_KEEP_COMMENTS);
    keys.set_double(sequenceKey, "target", luminance);
    keys.set_string(sequenceKey, "reference", reference);
    keys.set_string(sequenceKey, "time", Glib::DateTime::create_now_local().format("%Y-%m-%d %H:%M:%S"));
    Glib::file_set_contents(targetsFile(), keys.to_data());
  }
  catch (const Glib::Error& e)
  {
    printf("TTSeriesExposure: unable to write %s: %s\n", targetsFile().c_str(), e.what().c_str());
  }
}

void TTSeriesExposure::setInfo(const Glib::ustring& action)
{
  Glib::ustring text = action;
  if (sequenceReady)
  {
    double target;
    Glib::ustring reference;
    Glib::ustring targetText = readTarget(target, reference)
      ? Glib::ustring::compose(M("TT_SERIESEXP_TARGET"), Glib::ustring::format(std::fixed, std::setprecision(4), target), reference)
      : M("TT_SERIESEXP_NO_TARGET_YET");
    text += (text.empty() ? "" : "\n") + sequenceText + "\n" + targetText;
  }
  lbInfo->set_text(text);
  TT_LOG("TTSeriesExposure: %s\n", text.c_str());
}

// ---------------------------------------------------------------------------------------

void TTSeriesExposure::react(FakeProcEvent ev)
{
  if ((ev == FakeEvExifTransmitted) && getExpander()->getEnabled())
    startSequence();
}

void TTSeriesExposure::adjusterChanged(Adjuster* a, double newval)
{
  esbySettings().SeriesExpGap = adjGap->getValue();
  if (scan)
    sequenceScanned(); // the parts depend on the gap
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
