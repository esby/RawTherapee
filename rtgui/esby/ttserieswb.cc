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
#include "multilangmgr.h"
#include "variable.h"
#include "wbprovider.h"
#include "tools/whitebalance.h"
#include "rtengine/colortemp.h"
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

// ranges of the adjusters (a learned value outside them is refused, it was silently clamped)
static const double SERIESWB_MIN_MIRED = -100.0;
static const double SERIESWB_MAX_MIRED = 100.0;
static const double SERIESWB_MIN_GREEN_FACTOR = 0.5;
static const double SERIESWB_MAX_GREEN_FACTOR = 2.0;

TTSeriesWB::TTSeriesWB() : FoldableToolPanel(this, "TTSeriesWB", M("TT_SERIESWB_LABEL"), false, true)
{
  whitebalance = nullptr;

  // positive shift: lower temperature, cooler rendering (ex: 5044 K + 15.4 mireds = 4679 K)
//  adjMired = Gtk::manage(new Adjuster(M("TT_SERIESWB_MIRED"), SERIESWB_MIN_MIRED, SERIESWB_MAX_MIRED, 0.5, 0.0));
  // step 0.1: with 0.5, a learned shift was rounded enough to change the reference image itself
  adjMired = Gtk::manage(new Adjuster(M("TT_SERIESWB_MIRED"), SERIESWB_MIN_MIRED, SERIESWB_MAX_MIRED, 0.1, 0.0));
  adjMired->setAdjusterListener(this);
  adjMired->set_tooltip_text(M("TT_SERIESWB_MIRED_TOOLTIP"));
  pack_start(*adjMired, Gtk::PACK_SHRINK, 0);

//  adjGreen = Gtk::manage(new Adjuster(M("TT_SERIESWB_GREEN"), 0.8, 1.25, 0.001, 1.0));
  adjGreen = Gtk::manage(new Adjuster(M("TT_SERIESWB_GREEN"), SERIESWB_MIN_GREEN_FACTOR, SERIESWB_MAX_GREEN_FACTOR, 0.001, 1.0));
  adjGreen->setAdjusterListener(this);
  adjGreen->set_tooltip_text(M("TT_SERIESWB_GREEN_TOOLTIP"));
  pack_start(*adjGreen, Gtk::PACK_SHRINK, 0);

  Gtk::HBox* flashBox = Gtk::manage(new Gtk::HBox());
  flashBox->set_spacing(4);
  lbFlashOnly = Gtk::manage(new Gtk::Label(M("TT_SERIESWB_FLASH_ONLY")));
  cbFlashOnly = Gtk::manage(new Gtk::CheckButton());
  cbFlashOnly->set_active(true);
  flashBox->pack_start(*lbFlashOnly, Gtk::PACK_SHRINK, 0);
  flashBox->pack_end(*cbFlashOnly, Gtk::PACK_SHRINK, 0);
  pack_start(*flashBox, Gtk::PACK_SHRINK, 0);

  // the tint is learned only on demand: a flash mixed with ambient light shifts the temperature,
  // a tint correction (green / magenta) is only needed for some fluorescent or LED lights
  Gtk::HBox* learnTintBox = Gtk::manage(new Gtk::HBox());
  learnTintBox->set_spacing(4);
  lbLearnTint = Gtk::manage(new Gtk::Label(M("TT_SERIESWB_LEARN_TINT")));
  cbLearnTint = Gtk::manage(new Gtk::CheckButton());
  cbLearnTint->set_active(false);
  learnTintBox->pack_start(*lbLearnTint, Gtk::PACK_SHRINK, 0);
  learnTintBox->pack_end(*cbLearnTint, Gtk::PACK_SHRINK, 0);
  pack_start(*learnTintBox, Gtk::PACK_SHRINK, 0);

  Gtk::HBox* buttonBox = Gtk::manage(new Gtk::HBox());
  buttonBox->set_spacing(4);
  btApply = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_APPLY")));
  btApply->set_tooltip_text(M("TT_SERIESWB_APPLY_TOOLTIP"));
  btLearn = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_LEARN")));
  btLearn->set_tooltip_text(M("TT_SERIESWB_LEARN_TOOLTIP"));
  buttonBox->pack_start(*btApply, Gtk::PACK_EXPAND_WIDGET, 0);
  buttonBox->pack_start(*btLearn, Gtk::PACK_EXPAND_WIDGET, 0);
  pack_start(*buttonBox, Gtk::PACK_SHRINK, 0);

  // esbywb server: the value is declared per folder, the folders below inherit it
  Gtk::HBox* serverBox = Gtk::manage(new Gtk::HBox());
  serverBox->set_spacing(4);
  btSetFolder = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_SET_FOLDER")));
  btSetFolder->set_tooltip_text(M("TT_SERIESWB_SET_FOLDER_TOOLTIP"));
  btSetParent = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_SET_PARENT")));
  btSetParent->set_tooltip_text(M("TT_SERIESWB_SET_PARENT_TOOLTIP"));
  btUnset = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_UNSET")));
  btUnset->set_tooltip_text(M("TT_SERIESWB_UNSET_TOOLTIP"));
  serverBox->pack_start(*btSetFolder, Gtk::PACK_EXPAND_WIDGET, 0);
  serverBox->pack_start(*btSetParent, Gtk::PACK_EXPAND_WIDGET, 0);
  serverBox->pack_start(*btUnset, Gtk::PACK_EXPAND_WIDGET, 0);
  pack_start(*serverBox, Gtk::PACK_SHRINK, 0);

  lbServer = Gtk::manage(new Gtk::Label(""));
  lbServer->set_xalign(0.0);
  pack_start(*lbServer, Gtk::PACK_SHRINK, 0);

  lbInfo = Gtk::manage(new Gtk::Label(""));
  lbInfo->set_line_wrap(true);
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

  btApply->signal_clicked().connect([this]() { applyToCurrentImage(true); });
  btLearn->signal_clicked().connect([this]() { learnFromCurrentImage(); });
  btSetFolder->signal_clicked().connect([this]() { setFolderValue(false); });
  btSetParent->signal_clicked().connect([this]() { setFolderValue(true); });
  btUnset->signal_clicked().connect([this]() { unsetFolderValue(); });
  cbFlashOnly->signal_toggled().connect([this]() { saveSettings(); });
  cbLearnTint->signal_toggled().connect([this]() { saveSettings(); });

  exactMired = 0.0;
  exactGreen = 1.0;

  // the values of the last session; a ttp profile loaded afterwards has the priority
  loading = false;
  loadSettings();

  modified = false;
  client.reset(new EsbyWBClient());
  client->setStatusCallback([this](bool connected) {
    updateServerStatus();
    if (connected)
      requestForCurrentImage();
  });
  client->setEventCallback([this](const Glib::ustring& path) {
    // a value changed: the current image is concerned if its folder is the changed one or below it
    Glib::ustring folder = currentFolder();
    if (!folder.empty() && isBelow(folder, path))
      requestForCurrentImage();
  });
  updateServerStatus();
  client->start();
}

void TTSeriesWB::updateServerStatus()
{
  bool connected = client && client->isConnected();
  lbServer->set_text(connected ? M("TT_SERIESWB_SERVER_CONNECTED") : M("TT_SERIESWB_SERVER_UNAVAILABLE"));
  btSetFolder->set_sensitive(connected);
  btSetParent->set_sensitive(connected);
  btUnset->set_sensitive(connected);
}

// realpath: the server keys are absolute paths with the symbolic links resolved
Glib::ustring TTSeriesWB::realPath(const Glib::ustring& path)
{
  char* resolved = realpath(path.c_str(), nullptr);
  if (resolved == nullptr)
    return path;
  Glib::ustring result(resolved);
  free(resolved);
  return result;
}

bool TTSeriesWB::isBelow(const Glib::ustring& path, const Glib::ustring& folder)
{
  return (path == folder) || (folder == "/") || (path.compare(0, folder.size() + 1, folder + "/") == 0);
}

Glib::ustring TTSeriesWB::currentFolder()
{
  if (env == nullptr)
    return "";
  Glib::ustring file = currentFile();
  return file.empty() ? Glib::ustring() : realPath(Glib::path_get_dirname(file));
}

// asks the server the value of the folder of the current image, then the white balance it applied
// to the file, then applies (the answers come later, in the main loop)
void TTSeriesWB::requestForCurrentImage()
{
  if ((env == nullptr) || !getExpander()->getEnabled())
    return;
  Glib::ustring file = currentFile();
  if (file.empty())
    return;
  if (!client || !client->isConnected())
  {
    applyToCurrentImage(false);
    return;
  }

  Glib::ustring folder = currentFolder();
  client->get(folder, [this, file](bool ok, const EsbyWBRule& rule) {
    if (currentFile() != file) // another image was opened meanwhile
      return;
    if (!ok)
    {
      applyToCurrentImage(false);
      return;
    }
    // the tool shows the value of the folder (the local settings are kept for the cases without server).
    // no folder declares a value (empty source): the default value of the server only means that, the
    // values of the tool (last learned or set) are kept, they are the default value of the user.
    if (!rule.source.empty())
    {
      loading = true;
      adjMired->block(true);
      adjGreen->block(true);
      exactMired = rule.mired;
      exactGreen = rule.green;
      adjMired->setValue(rule.mired);
      adjGreen->setValue(rule.green);
      adjMired->block(false);
      adjGreen->block(false);
      cbFlashOnly->set_active(rule.flashOnly);
      loading = false;
    }
    currentSource = rule.source;
    modified = false;

    client->fileState(realPath(file), [this, file](bool ok, const EsbyWBFileState& state) {
      if (currentFile() != file)
        return;
      applyToCurrentImage(false, ok ? &state : nullptr);
    });
  });
}

// sends the values of the tool to the server, for the folder of the image or its parent.
// the server then notifies the change, and the image is updated by the event.
void TTSeriesWB::setFolderValue(bool parent)
{
  Glib::ustring folder = currentFolder();
  if (folder.empty() || !client || !client->isConnected())
    return;
  if (parent)
    folder = Glib::path_get_dirname(folder);
  client->set(folder, exactMired, exactGreen, cbFlashOnly->get_active(),
    [this, folder](bool ok, const Glib::ustring&) {
      setInfo(Glib::ustring::compose(M(ok ? "TT_SERIESWB_SET_DONE" : "TT_SERIESWB_SERVER_ERROR"), folder));
    });
}

void TTSeriesWB::unsetFolderValue()
{
  Glib::ustring folder = currentFolder();
  if (folder.empty() || !client || !client->isConnected())
    return;
  client->unset(folder, [this, folder](bool ok, const Glib::ustring&) {
    setInfo(Glib::ustring::compose(M(ok ? "TT_SERIESWB_UNSET_DONE" : "TT_SERIESWB_UNSET_NONE"), folder));
  });
}

void TTSeriesWB::loadSettings()
{
  loading = true;
  const EsbySettings& s = esbySettings();
  adjMired->block(true);
  adjGreen->block(true);
  exactMired = s.SeriesWBMired;
  exactGreen = s.SeriesWBGreen;
  adjMired->setValue(s.SeriesWBMired);
  adjGreen->setValue(s.SeriesWBGreen);
  adjMired->block(false);
  adjGreen->block(false);
  cbFlashOnly->set_active(s.SeriesWBFlashOnly);
  cbLearnTint->set_active(s.SeriesWBLearnTint);
  getExpander()->setEnabled(s.SeriesWBEnabled);
  loading = false;
}

// kept in the options (written when RawTherapee is closed, or right away by "learn")
void TTSeriesWB::saveSettings()
{
  if (loading)
    return;
  EsbySettings& s = esbySettings();
  s.SeriesWBEnabled = getExpander()->getEnabled();
  s.SeriesWBMired = exactMired;
  s.SeriesWBGreen = exactGreen;
  s.SeriesWBFlashOnly = cbFlashOnly->get_active();
  s.SeriesWBLearnTint = cbLearnTint->get_active();
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
}

void TTSeriesWB::deployLate()
{
}

void TTSeriesWB::enabledChanged()
{
  saveSettings();
  if (env == nullptr) // the tool is not deployed yet (settings loaded in the constructor)
    return;
  if (getExpander()->getEnabled())
    requestForCurrentImage();
}

Glib::ustring TTSeriesWB::currentFile()
{
  return env->getVarAsString("Fname");
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

void TTSeriesWB::computeTarget(double camTemp, double camGreen, int& temp, double& green)
{
  double mired = 1000000.0 / camTemp + exactMired;
  double t = (mired > 0.0) ? 1000000.0 / mired : SERIESWB_MAXTEMP;
  temp = (int) std::lround(std::max<double>(SERIESWB_MINTEMP, std::min<double>(SERIESWB_MAXTEMP, t)));
  green = std::max(SERIESWB_MINGREEN, std::min(SERIESWB_MAXGREEN, camGreen * exactGreen));
}

// the rounding of RawTherapee must not make an image, for instance the reference of a learned
// shift, look different from its series
bool TTSeriesWB::sameWB(int t1, double g1, int t2, double g2)
{
//  return (std::abs(t1 - t2) <= 1) && (std::fabs(g1 - g2) <= 0.001);
  if ((t1 <= 0) || (t2 <= 0) || (g2 <= 0.0))
    return false;
//  return (std::fabs(1000000.0 / t1 - 1000000.0 / t2) <= 0.15) && (std::fabs(g1 / g2 - 1.0) <= 0.002);
  // the exact shift is kept: only the rounding of RawTherapee remains, the temperature in integer
  // kelvins (1 K covers it at any temperature, a tolerance in mireds does not at low temperatures)
  // and the tint shown with 3 decimals
  return (std::abs(t1 - t2) <= 1) && (std::fabs(g1 / g2 - 1.0) <= 0.001);
}

void TTSeriesWB::setInfo(const Glib::ustring& text)
{
  lbInfo->set_text(text);
  TT_LOG("TTSeriesWB: %s\n", text.c_str());
}

// force: "apply to this image" button, the image follows the series again whatever its white balance
// serverState: white balance applied to the file according to the server (the image follows the
// series if it still has it), nullptr without server
void TTSeriesWB::applyToCurrentImage(bool force, const EsbyWBFileState* serverState)
{
  if (!getExpander()->getEnabled() && !force)
    return;

  Glib::ustring file = currentFile();
  WBParams wb;
  if (file.empty() || !getCurrentWB(wb))
  {
    setInfo(M("TT_SERIESWB_NO_IMAGE"));
    return;
  }

  double camTemp, camGreen;
  if (!getCameraWB(wb, camTemp, camGreen))
  {
    setInfo(M("TT_SERIESWB_NO_CAMERA_WB"));
    return;
  }

  if (!force && cbFlashOnly->get_active() && !flashFired())
  {
    setInfo(M("TT_SERIESWB_NO_FLASH"));
    return;
  }

  int temp;
  double green;
  computeTarget(camTemp, camGreen, temp, green);

  auto it = applied.find(file);
  bool follows = force
              || (wb.method == "Camera")
              || ((it != applied.end()) && sameWB(wb.temperature, wb.green, it->second.first, it->second.second))
              || ((serverState != nullptr) && serverState->known
                  && sameWB(wb.temperature, wb.green, serverState->temperature, serverState->green))
              || ((wb.method == "Custom") && sameWB(wb.temperature, wb.green, temp, green));

  if (!wb.enabled || !follows)
  {
    setInfo(Glib::ustring::compose(M("TT_SERIESWB_MANUAL"), wb.temperature));
    return;
  }

  if (!((wb.method == "Custom") && sameWB(wb.temperature, wb.green, temp, green)))
    whitebalance->setWB(temp, green);

  applied[file] = std::make_pair(temp, green);
  Glib::ustring origin;
  if (client && client->isConnected())
  {
    client->applied(realPath(file), temp, green, currentSource);
    origin = modified ? M("TT_SERIESWB_MODIFIED")
           : currentSource.empty() ? M("TT_SERIESWB_SOURCE_TOOL")
           : Glib::ustring::compose(M("TT_SERIESWB_SOURCE"), currentSource);
  }
  setInfo(Glib::ustring::compose(M("TT_SERIESWB_APPLIED"),
                                 (int) std::lround(camTemp), temp,
                                 Glib::ustring::format(std::fixed, std::setprecision(1), exactMired))
          + (origin.empty() ? Glib::ustring() : "\n" + origin));
}

// mired = mired(current white balance) - mired(camera); green = current green / camera green
void TTSeriesWB::learnFromCurrentImage()
{
  Glib::ustring file = currentFile();
  WBParams wb;
  double camTemp, camGreen;
  if (file.empty() || !getCurrentWB(wb) || !getCameraWB(wb, camTemp, camGreen) || (wb.temperature <= 0))
  {
    setInfo(M("TT_SERIESWB_NO_IMAGE"));
    return;
  }

  // "Camera" method: the engine applies the camera multipliers, the temperature and tint shown by
  // the white balance tool may still be the values of the profile (ex: a profile applied before the
  // image data was available). The current white balance is then the camera one.
  if (wb.method == "Camera")
  {
    wb.temperature = (int) std::lround(camTemp);
    wb.green = camGreen;
  }

  double mired = 1000000.0 / wb.temperature - 1000000.0 / camTemp;
//  double green = wb.green / camGreen;
  double green = cbLearnTint->get_active() ? wb.green / camGreen : 1.0;

  // rounded to the precision of the adjusters: the tool, the server and the reference image use the
  // same value (the reference keeps its white balance, see sameWB)
//  mired = std::round(mired * 10.0) / 10.0;
//  green = std::round(green * 1000.0) / 1000.0;
  // no rounding anymore: the exact values are kept (exactMired, exactGreen), the adjusters show them

  // the values of the current and camera white balances are shown, so the result can be checked
  Glib::ustring values = Glib::ustring::compose(M("TT_SERIESWB_VALUES"),
                                                wb.temperature, Glib::ustring::format(std::fixed, std::setprecision(3), wb.green),
                                                (int) std::lround(camTemp), Glib::ustring::format(std::fixed, std::setprecision(3), camGreen),
                                                wb.method,
                                                (wb.observer == rtengine::StandardObserver::TEN_DEGREES) ? "10" : "2");

  if ((mired < SERIESWB_MIN_MIRED) || (mired > SERIESWB_MAX_MIRED)
  || (green < SERIESWB_MIN_GREEN_FACTOR) || (green > SERIESWB_MAX_GREEN_FACTOR))
  {
    setInfo(Glib::ustring::compose(M("TT_SERIESWB_OUT_OF_RANGE"),
                                   Glib::ustring::format(std::fixed, std::setprecision(1), mired),
                                   Glib::ustring::format(std::fixed, std::setprecision(3), green))
            + "\n" + values);
    return;
  }

  // the adjusters are set without re-applying: the current image already has this white balance
  adjMired->block(true);
  adjGreen->block(true);
  exactMired = mired;
  exactGreen = green;
  adjMired->setValue(mired);
  adjGreen->setValue(green);
  adjMired->block(false);
  adjGreen->block(false);

  applied[file] = std::make_pair(wb.temperature, wb.green);

  // an explicit action: written right away, it must not be lost if RawTherapee is not closed properly
  saveSettings();
  Options::save();
  // with the server, the learned value becomes the value of the folder of the image (the shift of
  // a series is what is learned), and the image follows it. It was only a preview before, and the
  // value of the folder replaced it when the image was opened again.
  modified = false;
  if (client && client->isConnected())
  {
    Glib::ustring folder = currentFolder();
    client->set(folder, mired, green, cbFlashOnly->get_active(), nullptr);
    client->applied(realPath(file), wb.temperature, wb.green, folder);
    currentSource = folder;
    setInfo(Glib::ustring::compose(M("TT_SERIESWB_LEARNED"),
                                   Glib::ustring::format(std::fixed, std::setprecision(1), mired),
                                   Glib::ustring::format(std::fixed, std::setprecision(3), green))
            + "\n" + Glib::ustring::compose(M("TT_SERIESWB_SET_DONE"), folder)
            + "\n" + values);
    return;
  }

  setInfo(Glib::ustring::compose(M("TT_SERIESWB_LEARNED"),
                                 Glib::ustring::format(std::fixed, std::setprecision(1), mired),
                                 Glib::ustring::format(std::fixed, std::setprecision(3), green))
          + "\n" + M("TT_SERIESWB_LEARNED_LOCAL")
          + "\n" + values);
}

void TTSeriesWB::adjusterChanged(Adjuster* a, double newval)
{
  // a value chosen with the adjuster: its rounding is wanted
  if (a == adjMired)
    exactMired = adjMired->getValue();
  else if (a == adjGreen)
    exactGreen = adjGreen->getValue();
  saveSettings();
  // with the server, the change is a preview until it is sent with "set for this folder"
  modified = client && client->isConnected();
  // the current image is updated only if it follows the series (see applyToCurrentImage)
  applyToCurrentImage(false);
}

void TTSeriesWB::react(FakeProcEvent ev)
{
  // FakeEvExifTransmitted: the exif data (flash) and the file name are known, TTTweaker has
  // already reacted (it is registered before this tool).
  if ((ev == FakeEvExifTransmitted) || (ev == FakeEvProfileChanged))
    requestForCurrentImage(); // without server, applies the values of the tool
}

Glib::ustring TTSeriesWB::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(getExpander()->getEnabled() ? "1" : "0");
  Glib::ustring s_mired = getToolName() + ":" + "mired " + Glib::Ascii::dtostr(exactMired);
  Glib::ustring s_green = getToolName() + ":" + "green " + Glib::Ascii::dtostr(exactGreen);
  Glib::ustring s_flash = getToolName() + ":" + "flash_only " + std::string(cbFlashOnly->get_active() ? "1" : "0");
  Glib::ustring s_learn_tint = getToolName() + ":" + "learn_tint " + std::string(cbLearnTint->get_active() ? "1" : "0");

//  return s_active + "\n" + s_mired + "\n" + s_green + "\n" + s_flash + "\n" + s_learn_tint + "\n";
  // the shift and tint factor are working values (they change with the series), not a layout:
  // they are kept in the options and on the server, not in the ttp profiles. A profile loaded at
  // startup reset the learned shift to the value it was saved with.
  return s_active + "\n" + s_flash + "\n" + s_learn_tint + "\n";
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
          // ignored: the "mired" and "green" lines of the older profiles must not reset the working values
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
          else if (key == "learn_tint")
            cbLearnTint->set_active(value == "1");
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
  saveSettings();
}
