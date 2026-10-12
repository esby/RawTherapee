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

#include <gtkmm.h>
#include "toolpanel.h"
#include "guiutils.h"
#include "widgets/basic/adjuster.h"
#include "rtengine/procparams.h"

class WhiteBalance;

// series white balance, on the model of TTSeriesExposure:
//   white balance of an image = white balance of the camera + shift of the series + gap of the image
// - the shift of the series (mireds, tint factor, blue/red equalizer) is shared by TTShare (section
//   "White Balance", temperature shifted in mireds): a change of the white balance of an image, or of the
//   series sliders here, is shared live with the images of the series;
// - the gap of the image (mireds) is never shared: none, computed from the reference of the series (shutter
//   time: the ambient light adds up with the exposure; its own value in high speed sync), or manual.
// The reference of the series (shared variable wb.reference on the series folder) holds the shutter time
// and the gap of the reference image, the coefficient of the exposure and the gap of the high speed sync.
// The gap and its state are kept by the server (file data, namespace wb): TTShare gives the gap back to
// an image which lost its values. "Flash only": a photo without flash takes no part in the white balance
// of the series (its changes are not shared, it receives none).
// The switch of the tool turns it on; turned off, it does nothing (the gaps applied stay in the images).
class TTSeriesWB : public ToolParamBlock, public AdjusterListener, public FoldableToolPanel
{
protected:
    // the reference of the series, as kept in wb.reference (JSON)
    struct Reference
    {
        bool valid = false;
        Glib::ustring raw;
        double shutter = 0.0;      // shutter time of the reference image (s), 0 if unknown
        double gap = 0.0;          // its own gap (mireds) when it was set
        double k = 0.0;            // mireds per second of exposure (ambient light)
        bool hasHss = false;
        double hss = 0.0;          // gap of the images in high speed sync (mireds)
        double sync = 1.0 / 250.0; // shutter times up to this one are high speed sync
        bool isHss(double t) const { return (t > 0.0) && (t <= sync * 1.05); }
        static Reference parse(const Glib::ustring& text);
        Glib::ustring serialize() const;
    };

    Adjuster* adjMired;  // shift of the series (mireds): image - camera - gap
    Adjuster* adjGreen;  // tint factor of the series
    Adjuster* adjEqual;  // blue/red equalizer of the series
    Adjuster* adjGap;    // gap of the image (mireds)
    Gtk::Label* lbState; // none, computed, manual
    Gtk::Button* btCompute;
    Gtk::Button* btReset;
    Gtk::Button* btReference;
    Gtk::Button* btNoReference;
    Gtk::Button* btModel;
    Adjuster* adjSync;
    Gtk::CheckButton* cbFlashOnly;
    Gtk::Label* lbInfo;
    Gtk::EventBox* ebInfo; // clicking on the status line copies it to the clipboard

    WhiteBalance* whitebalance;

    Glib::ustring series;     // series folder of the current image
    Glib::ustring stateFile;  // image the state below belongs to
    bool stateLoaded;
    double gap;
    Glib::ustring state;      // none, computed, manual
    Reference reference;
    int generation;
    bool sharedConnected;
    sigc::connection paramsConnection;
    bool updatingWidgets;
    bool loading;

    Glib::ustring originalFile();
    bool flashFired();
    bool takesPart();         // the image takes part in the white balance of the series (flash only)
    double shutter();
    bool getCurrentWB(rtengine::procparams::WBParams& wb);
    bool getCameraWB(const rtengine::procparams::WBParams& wb, double& temp, double& green);
    // the white balance of the image relative to the camera: mireds, tint factor, equalizer
    bool relativeWB(double& mired, double& green, double& equal);
    // sets the white balance from the camera: shared (shift of the series) or not (gap of the image)
    void setRelativeWB(double mired, double green, double equal, bool share);

    void imageOpened();
    void loadReference(bool computeAfter);
    bool isReference();
    bool computeGap(double& g, Glib::ustring& why);
    void computeAndApply(bool explicitly);
    void setGap(double g, const Glib::ustring& newState, bool apply);
    void saveState();
    void showState();
    void setReferenceHere();
    void publishReference(const Reference& ref, const Glib::ustring& message);
    void removeReference();
    void modelFromImage();
    void observe(const Glib::ustring& kind);
    void setInfo(const Glib::ustring& action);

public:
    TTSeriesWB();
    ~TTSeriesWB() override;
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
