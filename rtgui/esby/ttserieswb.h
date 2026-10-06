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
#pragma once

#include <map>
#include <utility>
#include <gtkmm.h>
#include "toolpanel.h"
#include "guiutils.h"
#include "widgets/basic/adjuster.h"
#include "rtengine/procparams.h"

class WhiteBalance;

// series white balance, step 1 of SPEC_series_wb.md (without server):
// applies a shift (in mireds, plus a tint factor) to the white balance of the camera,
// for the photos taken with a flash. The images whose white balance was changed by hand
// are left untouched.
class TTSeriesWB : public ToolParamBlock, public AdjusterListener, public FoldableToolPanel
{
protected:
    Adjuster* adjMired;
    Adjuster* adjGreen;
    Gtk::CheckButton* cbFlashOnly;
    Gtk::Label* lbFlashOnly;
    Gtk::Button* btApply;
    Gtk::Button* btLearn;
    Gtk::Label* lbInfo;

    WhiteBalance* whitebalance;

    // white balance applied by this tool, per file (session only, the server will keep it later)
    std::map<Glib::ustring, std::pair<int, double>> applied;

    bool getCameraWB(const rtengine::procparams::WBParams& wb, double& temp, double& green);
    bool getCurrentWB(rtengine::procparams::WBParams& wb);
    bool flashFired();
    Glib::ustring currentFile();
    void computeTarget(double camTemp, double camGreen, int& temp, double& green);
    bool sameWB(int t1, double g1, int t2, double g2);
    void applyToCurrentImage(bool force);
    void learnFromCurrentImage();
    void setInfo(const Glib::ustring& text);

public:
    TTSeriesWB();
    void deploy();
    void deployLate();
    void react(FakeProcEvent ev);
    void enabledChanged();
    bool canBeEnabled() {return true;}

    void adjusterChanged(Adjuster* a, double newval) override;

// implement these to load / save ttp profiles
    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};
