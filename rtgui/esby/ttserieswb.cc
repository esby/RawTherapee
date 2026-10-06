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
#include "multilangmgr.h"
#include "variable.h"
#include "wbprovider.h"
#include "tools/whitebalance.h"
#include <cmath>
#include <iomanip>
#include <fstream>
#include <sstream>

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
  adjMired = Gtk::manage(new Adjuster(M("TT_SERIESWB_MIRED"), SERIESWB_MIN_MIRED, SERIESWB_MAX_MIRED, 0.5, 0.0));
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

  Gtk::HBox* buttonBox = Gtk::manage(new Gtk::HBox());
  buttonBox->set_spacing(4);
  btApply = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_APPLY")));
  btApply->set_tooltip_text(M("TT_SERIESWB_APPLY_TOOLTIP"));
  btLearn = Gtk::manage(new Gtk::Button(M("TT_SERIESWB_LEARN")));
  btLearn->set_tooltip_text(M("TT_SERIESWB_LEARN_TOOLTIP"));
  buttonBox->pack_start(*btApply, Gtk::PACK_EXPAND_WIDGET, 0);
  buttonBox->pack_start(*btLearn, Gtk::PACK_EXPAND_WIDGET, 0);
  pack_start(*buttonBox, Gtk::PACK_SHRINK, 0);

  lbInfo = Gtk::manage(new Gtk::Label(""));
  lbInfo->set_line_wrap(true);
  lbInfo->set_xalign(0.0);
  pack_start(*lbInfo, Gtk::PACK_SHRINK, 0);

  btApply->signal_clicked().connect([this]() { applyToCurrentImage(true); });
  btLearn->signal_clicked().connect([this]() { learnFromCurrentImage(); });
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
  if (getExpander()->getEnabled())
    applyToCurrentImage(false);
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
  double mired = 1000000.0 / camTemp + adjMired->getValue();
  double t = (mired > 0.0) ? 1000000.0 / mired : SERIESWB_MAXTEMP;
  temp = (int) std::lround(std::max<double>(SERIESWB_MINTEMP, std::min<double>(SERIESWB_MAXTEMP, t)));
  green = std::max(SERIESWB_MINGREEN, std::min(SERIESWB_MAXGREEN, camGreen * adjGreen->getValue()));
}

bool TTSeriesWB::sameWB(int t1, double g1, int t2, double g2)
{
  return (std::abs(t1 - t2) <= 1) && (std::fabs(g1 - g2) <= 0.001);
}

void TTSeriesWB::setInfo(const Glib::ustring& text)
{
  lbInfo->set_text(text);
  TT_LOG("TTSeriesWB: %s\n", text.c_str());
}

// force: "apply to this image" button, the image follows the series again whatever its white balance
void TTSeriesWB::applyToCurrentImage(bool force)
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
              || ((wb.method == "Custom") && sameWB(wb.temperature, wb.green, temp, green));

  if (!wb.enabled || !follows)
  {
    setInfo(Glib::ustring::compose(M("TT_SERIESWB_MANUAL"), wb.temperature));
    return;
  }

  if (!((wb.method == "Custom") && sameWB(wb.temperature, wb.green, temp, green)))
    whitebalance->setWB(temp, green);

  applied[file] = std::make_pair(temp, green);
  setInfo(Glib::ustring::compose(M("TT_SERIESWB_APPLIED"),
                                 (int) std::lround(camTemp), temp,
                                 Glib::ustring::format(std::fixed, std::setprecision(1), adjMired->getValue())));
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
  double green = wb.green / camGreen;

  // the values of the current and camera white balances are shown, so the result can be checked
  Glib::ustring values = Glib::ustring::compose(M("TT_SERIESWB_VALUES"),
                                                wb.temperature, Glib::ustring::format(std::fixed, std::setprecision(3), wb.green),
                                                (int) std::lround(camTemp), Glib::ustring::format(std::fixed, std::setprecision(3), camGreen),
                                                wb.method);

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
  adjMired->setValue(mired);
  adjGreen->setValue(green);
  adjMired->block(false);
  adjGreen->block(false);

  applied[file] = std::make_pair(wb.temperature, wb.green);
  setInfo(Glib::ustring::compose(M("TT_SERIESWB_LEARNED"),
                                 Glib::ustring::format(std::fixed, std::setprecision(1), mired),
                                 Glib::ustring::format(std::fixed, std::setprecision(3), green))
          + "\n" + values);
}

void TTSeriesWB::adjusterChanged(Adjuster* a, double newval)
{
  // the current image is updated only if it follows the series (see applyToCurrentImage)
  applyToCurrentImage(false);
}

void TTSeriesWB::react(FakeProcEvent ev)
{
  // FakeEvExifTransmitted: the exif data (flash) and the file name are known, TTTweaker has
  // already reacted (it is registered before this tool).
  if ((ev == FakeEvExifTransmitted) || (ev == FakeEvProfileChanged))
    applyToCurrentImage(false);
}

Glib::ustring TTSeriesWB::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(getExpander()->getEnabled() ? "1" : "0");
  Glib::ustring s_mired = getToolName() + ":" + "mired " + Glib::Ascii::dtostr(adjMired->getValue());
  Glib::ustring s_green = getToolName() + ":" + "green " + Glib::Ascii::dtostr(adjGreen->getValue());
  Glib::ustring s_flash = getToolName() + ":" + "flash_only " + std::string(cbFlashOnly->get_active() ? "1" : "0");

  return s_active + "\n" + s_mired + "\n" + s_green + "\n" + s_flash + "\n";
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
          else if (key == "mired")
          {
            adjMired->block(true);
            adjMired->setValue(Glib::Ascii::strtod(value));
            adjMired->block(false);
          }
          else if (key == "green")
          {
            adjGreen->block(true);
            adjGreen->setValue(Glib::Ascii::strtod(value));
            adjGreen->block(false);
          }
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
