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
#include "esbywbclient.h"

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
    Adjuster* adjEqual; // factor of the blue/red equalizer
    Gtk::CheckButton* cbFlashOnly;
    Gtk::Label* lbLearnTint;
    Gtk::CheckButton* cbLearnTint;
    Gtk::Label* lbFlashOnly;
    Gtk::Button* btApply;
    Gtk::Button* btLearn;
    Gtk::Label* lbInfo;
    Gtk::EventBox* ebInfo; // clicking on the status line copies it to the clipboard

    // esbywb server (step 3): value per folder, with inheritance
    std::unique_ptr<EsbyWBClient> client;
    Gtk::Button* btSetFolder;
    Gtk::Button* btSetParent;
    Gtk::Button* btUnset;
    Gtk::Label* lbServer;
    Glib::ustring currentSource; // folder defining the value of the current image (empty: default)
    bool modified;               // the shift was changed in the tool and not sent to the server

    WhiteBalance* whitebalance;

    // exact values of the shift and tint factor: the adjusters round their value to their
    // precision, they only show these values (moving an adjuster sets them to its value)
    double exactMired;
    double exactGreen;
    double exactEqual;

    // modes: shift from the camera white balance, or auto (fixed white balance, exposure model)
    enum { MODE_SHIFT = 0, MODE_AUTO = 1 };
    int mode;
    EsbyWBAuto autoModel;
    MyComboBoxText* cbMode;
    Adjuster* adjSync;          // synchronization speed (1/x s): high speed sync above
    Gtk::Label* lbAuto;         // description of the auto model
    void modeChanged();
    void updateModeWidgets();
    double shutter();           // shutter time of the current image (s), 0 if unknown
    Glib::ustring describeAuto();

    // white balance applied by this tool, per file (session only, the server will keep it later)
    struct AppliedWB
    {
        int temperature;
        double green;
        double equal;
    };
    std::map<Glib::ustring, AppliedWB> applied;

    bool getCameraWB(const rtengine::procparams::WBParams& wb, double& temp, double& green);
    bool getCurrentWB(rtengine::procparams::WBParams& wb);
    bool flashFired();
    Glib::ustring currentFile();
    bool computeTarget(double camTemp, double camGreen, int& temp, double& green, double& equal);
    bool sameWB(int t1, double g1, double e1, int t2, double g2, double e2);
    void setWhiteBalance(int temp, double green, double equal);
    void applyToCurrentImage(bool force, const EsbyWBFileState* serverState = nullptr);
    void requestForCurrentImage();
    void setFolderValue(bool seriesRoot);
    Glib::ustring findSeriesRoot(const Glib::ustring& folder);
    Glib::ustring pendingAction; // action shown by the next status (ex: kept after a learn)
    void unsetFolderValue();
    void updateServerStatus();
    Glib::ustring currentFolder();
    Glib::ustring seriesFile();   // the original file (ESBY_ORIGIN) or the current one, links resolved
    bool fromOrigin();            // the original file given by ESBY_ORIGIN is used
    static Glib::ustring realPath(const Glib::ustring& path);
    static bool isBelow(const Glib::ustring& path, const Glib::ustring& folder);
    void learnFromCurrentImage();
    // sends to the server the correction of the current image (kind: learn, saved-manual, saved-series,
    // saved-camera) with its exposure, for the analysis of the link between the shift and the exposure
    void observe(const Glib::ustring& kind);
    void observeFinal(const Glib::ustring& event); // saved or closed: kind from the white balance
    void setInfo(const Glib::ustring& text);
    // status line, always the same structure: what happened, the series values, the image and camera values
    void setStatus(const Glib::ustring& action, const rtengine::procparams::WBParams& wb, double camTemp, double camGreen,
                   const Glib::ustring& origin = Glib::ustring());
    void loadSettings();
    void saveSettings();
    bool loading;

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
