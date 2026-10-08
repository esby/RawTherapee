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
  sharedConnected = false;
  toggleOld = toggleNew = 0.0;
  toggleShowsNew = true;
  hasShownOffset = false;
  shownOffset = 0.0;
  offsetPending = false;
  focusConnected = false;

  // the pipette of the first tone curve of the exposure tool: values after the exposure
  // compensation, the brightness, the contrast and the highlight compression, before the curves
  setEditID(EUID_ToneCurve1, BT_IMAGEFLOAT);

  tbPipette = Gtk::manage(new Gtk::ToggleButton(M("TT_SERIESEXP_PIPETTE")));
  tbPipette->set_tooltip_text(M("TT_SERIESEXP_PIPETTE_TOOLTIP"));
  pack_start(*tbPipette, Gtk::PACK_SHRINK, 0);

  // external measuring service: reference, measure and apply buttons
  Gtk::FlowBox* serviceBox = Gtk::manage(new Gtk::FlowBox());
  serviceBox->set_selection_mode(Gtk::SELECTION_NONE);
  serviceBox->set_column_spacing(4);
  serviceBox->set_row_spacing(2);
  btReference = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_REFERENCE")));
  btReference->set_tooltip_text(M("TT_SERIESEXP_REFERENCE_TOOLTIP"));
  btMeasure = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_MEASURE")));
  btMeasure->set_tooltip_text(M("TT_SERIESEXP_MEASURE_TOOLTIP"));
  btApply = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_APPLY")));
  btApply->set_tooltip_text(M("TT_SERIESEXP_APPLY_TOOLTIP"));
  btApply->set_sensitive(false);
  serviceBox->add(*btReference);
  serviceBox->add(*btMeasure);
  serviceBox->add(*btApply);
  pack_start(*serviceBox, Gtk::PACK_SHRINK, 0);
  suggestedComp = 0.0;
  hasSuggestion = false;
  service.reset(new EsbyHttpClient());
  service->setAddress(esbySettings().SeriesExpService);
  const char* token = g_getenv("ESBY_EXPOSURE_TOKEN");
  if (token != nullptr)
    service->setToken(token);
  btReference->set_sensitive(service->isEnabled());
  btMeasure->set_sensitive(service->isEnabled());
  btReference->signal_clicked().connect([this]() { setReference(false, 0.0, 0.0); });
  btMeasure->signal_clicked().connect([this]() { measureCurrent(true); });
  btApply->signal_clicked().connect([this]() {
    if (hasSuggestion)
    {
      setExpComp(suggestedComp);
      writeApplied(Glib::path_get_basename(originalFile()), suggestedComp, sequenceOffset());
      hasSuggestion = false;
      btApply->set_sensitive(false);
      setInfo(Glib::ustring::compose(M("TT_SERIESEXP_APPLIED_SUGGESTION"),
                                     Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), suggestedComp)));
    }
  });

  adjGap = Gtk::manage(new Adjuster(M("TT_SERIESEXP_GAP"), 1.0, 30.0, 0.5, 10.0));
  adjGap->setAdjusterListener(this);
  adjGap->set_tooltip_text(M("TT_SERIESEXP_GAP_TOOLTIP"));
  adjGap->block(true);
  adjGap->setValue(esbySettings().SeriesExpGap);
  adjGap->block(false);
  pack_start(*adjGap, Gtk::PACK_SHRINK, 0);

  // offset of the sequence: set on the sequence of the image (esby server), seen by every instance
  adjOffset = Gtk::manage(new Adjuster(M("TT_SERIESEXP_OFFSET"), -2.0, 2.0, 0.05, 0.0));
  adjOffset->setAdjusterListener(this);
  adjOffset->set_tooltip_text(M("TT_SERIESEXP_OFFSET_TOOLTIP"));
  adjOffset->set_sensitive(false);
  pack_start(*adjOffset, Gtk::PACK_SHRINK, 0);
  btToggle = Gtk::manage(new Gtk::Button(M("TT_SERIESEXP_TOGGLE_OLD")));
  btToggle->set_tooltip_text(M("TT_SERIESEXP_TOGGLE_TOOLTIP"));
  btToggle->set_no_show_all(true);
  btToggle->signal_clicked().connect([this]() {
    toggleShowsNew = !toggleShowsNew;
    double comp = toggleShowsNew ? toggleNew : toggleOld;
    setExpComp(comp);
    // the shown value is the one kept. The record stays the new value: shown, the image is untouched
    // and follows the next offsets; the old value differs from it, as a change by hand would
    writeApplied(Glib::path_get_basename(originalFile()), toggleNew, sequenceOffset());
    btToggle->set_label(M(toggleShowsNew ? "TT_SERIESEXP_TOGGLE_OLD" : "TT_SERIESEXP_TOGGLE_NEW"));
  });
  pack_start(*btToggle, Gtk::PACK_SHRINK, 0);

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

// the sequence is known: ready, and given to the shared variables (a variable can be set on it)
void TTSeriesExposure::publishSequence()
{
  sequenceReady = true;
  if (env == nullptr)
    return;
  Glib::ustring::size_type hash = sequenceKey.find('#');
  env->sharedVariables()->setSequence(originalFile(),
                                      (hash == Glib::ustring::npos) ? Glib::ustring() : sequenceKey.substr(hash + 1));
  offsetChanged(); // the variables may already be loaded (same sequence): no reload, no signal
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
    publishSequence();
    setInfo("");
    measureCurrent(false);
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
    publishSequence();
    setInfo("");
    measureCurrent(false);
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
  publishSequence();
  setInfo("");
  measureCurrent(false);
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

  // with the service, ctrl + click designates the face of the reference (point in 0 - 1 of the image)
  if ((modifierKey & GDK_CONTROL_MASK) && service->isEnabled())
  {
    EditDataProvider* provider = getEditProvider();
    int w = 0, h = 0;
    provider->getImageSize(w, h);
    if ((w > 0) && (h > 0))
      setReference(true, (double) provider->posImage.x / w, (double) provider->posImage.y / h);
    return true;
  }

  // ctrl + click: the brightness of the reference image becomes the target of the sequence
  if (modifierKey & GDK_CONTROL_MASK)
  {
    // the target is kept without the offset of the sequence (it is applied on top of it)
    luminance /= std::pow(2.0, sequenceOffset());
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
  double ev = std::log2(target / luminance) + sequenceOffset();
  double comp = std::max(SERIESEXP_MIN_COMP, std::min(SERIESEXP_MAX_COMP, currentExpComp() + ev));
  setExpComp(comp);
  writeApplied(Glib::path_get_basename(originalFile()), comp, sequenceOffset());
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

// ---------------------------------------------------------------------------------------
// external measuring service
// ---------------------------------------------------------------------------------------

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

bool TTSeriesExposure::readServiceRef(ServiceRef& ref)
{
  ref = ServiceRef();
  if (sequenceKey.empty())
    return false;
  Glib::KeyFile keys;
  try
  {
    if (!Glib::file_test(targetsFile(), Glib::FILE_TEST_EXISTS) || !keys.load_from_file(targetsFile()))
      return false;
    if (!keys.has_group(sequenceKey) || !keys.has_key(sequenceKey, "service_raw"))
      return false;
    ref.raw = keys.get_string(sequenceKey, "service_raw");
    ref.comp = keys.has_key(sequenceKey, "service_comp") ? keys.get_double(sequenceKey, "service_comp") : 0.0;
    if (keys.has_key(sequenceKey, "service_point"))
    {
      std::vector<double> p = keys.get_double_list(sequenceKey, "service_point");
      if (p.size() == 2)
      {
        ref.hasPoint = true;
        ref.px = p[0];
        ref.py = p[1];
      }
    }
    ref.valid = !ref.raw.empty();
    return ref.valid;
  }
  catch (const Glib::Error&)
  {
    return false;
  }
}

void TTSeriesExposure::writeServiceRef(const ServiceRef& ref)
{
  Glib::KeyFile keys;
  try
  {
    if (Glib::file_test(targetsFile(), Glib::FILE_TEST_EXISTS))
      keys.load_from_file(targetsFile(), Glib::KEY_FILE_KEEP_COMMENTS);
    keys.set_string(sequenceKey, "service_raw", ref.raw);
    keys.set_double(sequenceKey, "service_comp", ref.comp);
    if (ref.hasPoint)
      keys.set_double_list(sequenceKey, "service_point", std::vector<double>{ref.px, ref.py});
    else if (keys.has_key(sequenceKey, "service_point"))
      keys.remove_key(sequenceKey, "service_point");
    keys.set_string(sequenceKey, "reference", Glib::path_get_basename(ref.raw));
    keys.set_string(sequenceKey, "time", Glib::DateTime::create_now_local().format("%Y-%m-%d %H:%M:%S"));
    Glib::file_set_contents(targetsFile(), keys.to_data());
  }
  catch (const Glib::Error& e)
  {
    printf("TTSeriesExposure: unable to write %s: %s\n", targetsFile().c_str(), e.what().c_str());
  }
}

// compensation applied by the tool to a file (an image is changed automatically only if it still has it)
bool TTSeriesExposure::readApplied(const Glib::ustring& file, double& comp, double* offset)
{
  Glib::KeyFile keys;
  try
  {
    if (!Glib::file_test(targetsFile(), Glib::FILE_TEST_EXISTS) || !keys.load_from_file(targetsFile()))
      return false;
    if (!keys.has_group("applied") || !keys.has_key("applied", file))
      return false;
    comp = keys.get_double("applied", file);
    if (offset != nullptr)
      *offset = (keys.has_group("applied_offset") && keys.has_key("applied_offset", file))
              ? keys.get_double("applied_offset", file) : 0.0;
    return true;
  }
  catch (const Glib::Error&)
  {
    return false;
  }
}

void TTSeriesExposure::writeApplied(const Glib::ustring& file, double comp, double offset)
{
  Glib::KeyFile keys;
  try
  {
    if (Glib::file_test(targetsFile(), Glib::FILE_TEST_EXISTS))
      keys.load_from_file(targetsFile(), Glib::KEY_FILE_KEEP_COMMENTS);
    keys.set_double("applied", file, comp);
    keys.set_double("applied_offset", file, offset);
    Glib::file_set_contents(targetsFile(), keys.to_data());
  }
  catch (const Glib::Error&) {}
}

// automatic exposure or histogram matching: each image gets its own exposure, the absolute
// compensation (reference + delta) does not hold anymore
bool TTSeriesExposure::exposureIsAutomatic()
{
  ProcParams pp;
  tonecurve->write(&pp);
  return pp.toneCurve.autoexp || pp.toneCurve.histmatching;
}

void TTSeriesExposure::setSuggestion(double comp, const Glib::ustring& text)
{
  suggestedComp = comp;
  hasSuggestion = true;
  btApply->set_sensitive(true);
  setInfo(text);
}

// the current image becomes the reference of the sequence: its face (at the point, or near the AF
// point, or the largest) for the service, and its exposure compensation
void TTSeriesExposure::setReference(bool hasPoint, double px, double py)
{
  if (!service->isEnabled() || !sequenceReady || (tonecurve == nullptr))
  {
    setInfo(M(sequenceReady ? "TT_SERIESEXP_NO_SERVICE" : "TT_SERIESEXP_SEQ_WAIT"));
    return;
  }
  ServiceRef ref;
  ref.raw = originalFile();
  ref.hasPoint = hasPoint;
  ref.px = px;
  ref.py = py;
  ref.comp = currentExpComp() - sequenceOffset(); // kept without the offset of the sequence

  cJSON* body = cJSON_CreateObject();
  cJSON_AddStringToObject(body, "session", sequenceKey.c_str());
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
    Glib::ustring st = status->valuestring;
    if (st != "ok")
    {
      setInfo(Glib::ustring::compose(M("TT_SERIESEXP_STATUS"), st));
      return;
    }
    writeServiceRef(ref);
    writeApplied(Glib::path_get_basename(file), ref.comp + sequenceOffset(), sequenceOffset());
    cJSON* choice = cJSON_GetObjectItem(answer, "choice");
    setInfo(Glib::ustring::compose(M("TT_SERIESEXP_REFERENCE_SET"),
                                   cJSON_IsString(choice) ? choice->valuestring : "",
                                   Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), ref.comp)));
  });
}

// measure of the current image by the service. manual: from the measure button (the application is
// then proposed even for an image changed by hand)
void TTSeriesExposure::measureCurrent(bool manual, bool retried)
{
  if (!service->isEnabled() || !sequenceReady || (tonecurve == nullptr))
    return;
  ServiceRef ref;
  if (!readServiceRef(ref))
  {
    if (manual)
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
  cJSON_AddStringToObject(body, "session", sequenceKey.c_str());
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
      cJSON_AddStringToObject(again, "session", sequenceKey.c_str());
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
      double delta = number("delta_ev", 0.0);
      double similarity = number("similarity", 0.0);
      double clipped = number("clipped_fraction", 0.0);
      double comp = std::max(SERIESEXP_MIN_COMP, std::min(SERIESEXP_MAX_COMP, ref.comp + delta + sequenceOffset()));
      Glib::ustring values = Glib::ustring::compose(M("TT_SERIESEXP_MEASURED"),
                                                    Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), delta),
                                                    Glib::ustring::format(std::fixed, std::setprecision(2), similarity),
                                                    Glib::ustring::format(std::fixed, std::setprecision(1), clipped * 100.0),
                                                    Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), comp));
      // untouched: the compensation applied by the tool, or 0 (default profile) without record
      double applied;
      double current = currentExpComp();
      bool untouched = readApplied(Glib::path_get_basename(file), applied) ? (std::fabs(current - applied) < 0.005)
                                                                           : (std::fabs(current) < 0.005);
      const EsbySettings& s = esbySettings();
      bool automatic = !manual && untouched && (similarity >= s.SeriesExpAutoSimilarity)
                    && (clipped <= s.SeriesExpMaxClipped) && (std::fabs(delta) <= s.SeriesExpMaxEv);
      if (automatic)
      {
        setExpComp(comp);
        writeApplied(Glib::path_get_basename(file), comp, sequenceOffset());
        hasSuggestion = false;
        btApply->set_sensitive(false);
        setInfo(M("TT_SERIESEXP_AUTO_APPLIED") + "\n" + values);
      }
      else
        setSuggestion(comp, M(untouched || manual ? "TT_SERIESEXP_SUGGESTED" : "TT_SERIESEXP_SUGGESTED_MANUAL") + "\n" + values);
      return;
    }

    // no face (person seen from the back): the AF area measure is only proposed, never applied
    cJSON* fallback = cJSON_GetObjectItem(answer, "fallback_af");
    if ((st == "no_face") && cJSON_IsObject(fallback))
    {
      cJSON* d = cJSON_GetObjectItem(fallback, "delta_ev");
      if (cJSON_IsNumber(d))
      {
        double comp = std::max(SERIESEXP_MIN_COMP, std::min(SERIESEXP_MAX_COMP, ref.comp + d->valuedouble + sequenceOffset()));
        setSuggestion(comp, Glib::ustring::compose(M("TT_SERIESEXP_SUGGESTED_AF"),
                                                   Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), d->valuedouble),
                                                   Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), comp)));
        return;
      }
    }
    // no_match: another person is probably on the AF point, no fallback
    setInfo(Glib::ustring::compose(M(st == "no_match" ? "TT_SERIESEXP_NO_MATCH" : "TT_SERIESEXP_STATUS"), st));
  });
}

// ---------------------------------------------------------------------------------------
// offset of the sequence
// ---------------------------------------------------------------------------------------

// shared variable exposure.offset of the image (0 when not set, or without server)
double TTSeriesExposure::sequenceOffset()
{
  if (env == nullptr)
    return 0.0;
  RtVariable* v = env->getVariableByName("exposure.offset");
  if ((v == nullptr) || !v->isDefined() || (v->getScope() == RtVariableScope::Internal) || (v->getScope() == RtVariableScope::Exif))
    return 0.0;
  return v->getAsDouble();
}

// the slider was moved: the offset is set on the sequence of the image; the server sends the change
// to every instance (this one included), offsetChanged() applies it
void TTSeriesExposure::offsetAdjusted()
{
  if (!sequenceReady || (env == nullptr))
    return;
  EsbyVarValue value;
  value.name = "exposure.offset";
  value.type = RT_VARIABLE_TYPE_DOUBLE;
  value.d = adjOffset->getValue();
  env->sharedVariables()->set(EsbySharedVariables::Where::Sequence, value);
}

// the shared variables of the image were (re)loaded: an image still having the compensation applied by
// the tool gets the new offset (old/new button to compare); an image changed by hand only gets a message
void TTSeriesExposure::offsetChanged()
{
  if ((env == nullptr) || (tonecurve == nullptr))
    return;
  EsbySharedVariables* shared = env->sharedVariables();
  adjOffset->set_sensitive(shared->isConnected() && sequenceReady);
  double offset = sequenceOffset();
  adjOffset->block(true);
  adjOffset->setValue(offset);
  adjOffset->block(false);
  // the offset is 0 while the variables of a new image or sequence are loading: nothing is done then
  if (!getExpander()->getEnabled() || !sequenceReady || !shared->isLoaded())
    return;
  if (exposureIsAutomatic())
  {
    if (std::fabs(offset - shownOffset) >= 0.005)
      setInfo(M("TT_SERIESEXP_AUTOMATIC"));
    return;
  }

  // the image is only recalculated in the active window: N instances would recalculate together,
  // the others do it when they get the focus
  Gtk::Window* window = dynamic_cast<Gtk::Window*>(get_toplevel());
  if ((window != nullptr) && !focusConnected)
  {
    // the window becomes active in the default handler of focus-in: the change is applied just after
    window->signal_focus_in_event().connect([this](GdkEventFocus*) {
      if (offsetPending)
        Glib::signal_idle().connect_once([this]() { if (offsetPending) offsetChanged(); });
      return false;
    }, true);
    focusConnected = true;
  }
  if ((window != nullptr) && !window->is_active())
  {
    offsetPending = true;
    setInfo(Glib::ustring::compose(M("TT_SERIESEXP_OFFSET_PENDING"),
                                   Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), offset)));
    return;
  }
  offsetPending = false;

  Glib::ustring file = originalFile();
  if (file != offsetFile)
  {
    offsetFile = file;
    hasShownOffset = false;
    btToggle->hide();
  }
  Glib::ustring name = Glib::path_get_basename(file);
  double applied = 0.0, appliedOffset = 0.0;
  double current = currentExpComp();
  // without record, an image at 0 (default profile) is taken as untouched, with no offset
  bool known = readApplied(name, applied, &appliedOffset);
  if (!hasShownOffset)
  {
    // first look at this image: the offset it was given. The same file may be open in another
    // instance, which records its own changes: this instance compares with what it shows
    shownOffset = (known && (std::fabs(current - applied) < 0.005)) ? appliedOffset : 0.0;
    hasShownOffset = true;
  }
  if (std::fabs(offset - shownOffset) < 0.005)
    return; // nothing new
  // untouched: the compensation of the tool without its offset, plus the offset shown here
  double base = known ? applied - appliedOffset : 0.0;
  if (std::fabs(current - (base + shownOffset)) >= 0.005)
  {
    setInfo(Glib::ustring::compose(M("TT_SERIESEXP_OFFSET_NOT_APPLIED"),
                                   Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), shownOffset),
                                   Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), offset)));
    return;
  }
  double comp = std::max(SERIESEXP_MIN_COMP, std::min(SERIESEXP_MAX_COMP, current - shownOffset + offset));
  shownOffset = offset;
  if (!btToggle->is_visible())
    toggleOld = current; // the value before the first change seen on this image
  toggleNew = comp;
  toggleShowsNew = true;
  setExpComp(comp);
  writeApplied(name, comp, offset);
  btToggle->set_label(M("TT_SERIESEXP_TOGGLE_OLD"));
  btToggle->show();
  setInfo(Glib::ustring::compose(M("TT_SERIESEXP_OFFSET_APPLIED"),
                                 Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), offset),
                                 Glib::ustring::format(std::showpos, std::fixed, std::setprecision(2), comp)));
}

void TTSeriesExposure::react(FakeProcEvent ev)
{
  if (!sharedConnected && (env != nullptr))
  {
    env->sharedVariables()->signalChanged().connect(sigc::mem_fun(*this, &TTSeriesExposure::offsetChanged));
    sharedConnected = true;
  }
  if ((ev == FakeEvExifTransmitted) && getExpander()->getEnabled())
    startSequence();
}

void TTSeriesExposure::adjusterChanged(Adjuster* a, double newval)
{
  if (a == adjOffset)
  {
    offsetAdjusted();
    return;
  }
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
