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
#include <memory>
#include <vector>
#include <gtkmm.h>
#include "toolpanel.h"
#include "guiutils.h"
#include "editcallbacks.h"
#include "widgets/basic/adjuster.h"
#include "esbyhttpclient.h"

class ToneCurve;

// series exposure: the gap of the exposure of an image with the reference image of its series.
//
// The exposure compensation shared by the series (a change of the compensation in an image, with
// "Share my changes" of TTShare) is handled by TTShare. This tool only adds to the compensation of the
// image its own gap (EV), never shared:
// - default: no gap;
// - computed: measured against the reference of the series, with the pipette (brightness of a
//   reference area) or by the external measuring service (face);
// - manual: set with the slider.
// The reference of the series (shared variable exposure.reference, set on the series folder) is only
// used to compute the gaps. The gap and its state are kept by the server (file data, namespace
// exposure): TTShare gives the gap back to an image which lost its values (pp3 removed).
// Turned off, the tool does nothing (the gaps already applied stay in the images).
class TTSeriesExposure : public ToolParamBlock, public FoldableToolPanel, public EditSubscriber, public AdjusterListener
{
protected:
    // the reference of the series, as kept in exposure.reference (JSON): raw file, its compensation and
    // its own gap when it was set, brightness under the pipette (target), point of the face (service)
    struct Reference
    {
        bool valid = false;
        Glib::ustring raw;
        double comp = 0.0;
        double gap = 0.0;
        bool hasTarget = false;
        double target = 0.0;
        bool hasPoint = false;
        double px = 0.0, py = 0.0;
        static Reference parse(const Glib::ustring& text);
        Glib::ustring serialize() const;
    };

    Gtk::ToggleButton* tbPipette;
    Gtk::Button* btReference;      // the current image becomes the reference (with the service)
    Gtk::Button* btNoReference;    // the current image is not the reference anymore
    Adjuster* adjSeries;           // compensation of the series: compensation of the image without its gap
    Adjuster* adjGap;              // gap of the current image (EV)
    Gtk::Label* lbState;           // default, computed, manual
    Gtk::Button* btMeasure;        // measure the current image again (service)
    Gtk::Button* btApply;          // apply the suggested gap
    Gtk::Button* btReset;          // back to no gap
    Gtk::Label* lbInfo;

    std::unique_ptr<EsbyHttpClient> service;
    ToneCurve* tonecurve;

    Glib::ustring series;          // numbered folder of the current image, links resolved
    Glib::ustring stateFile;       // image the state below belongs to
    bool stateLoaded;
    double gap;                    // gap applied to the current image
    Glib::ustring state;           // default, computed, manual
    Reference reference;
    int generation;
    bool hasSuggestion;
    double suggestedGap;
    bool sharedConnected;
    sigc::connection paramsConnection;
    bool updatingWidgets;

    Glib::ustring originalFile();  // ESBY_ORIGIN, or the current file

    bool measure(double& luminance);
    double currentExpComp();
    void setExpComp(double value);
    bool exposureIsAutomatic();
    static bool readAfPoint(const Glib::ustring& file, double& x, double& y);

    void imageOpened();
    void loadReference(bool measureAfter);
    bool isReference();
    void setGap(double g, const Glib::ustring& newState, bool apply);
    void saveState();
    void showState();
    void setReferenceHere(bool hasPoint, double px, double py, bool hasTarget, double target);
    void publishReference(const Reference& ref);
    void removeReference();
    void measureCurrent(bool manual, bool retried = false);
    void setSuggestion(double g, const Glib::ustring& text);
    void setInfo(const Glib::ustring& action);

public:
    TTSeriesExposure();
    ~TTSeriesExposure() override;

    void deploy();
    void deployLate();
    void react(FakeProcEvent ev);
    void enabledChanged() override;
    bool canBeEnabled() {return true;}

    // EditSubscriber
    void setEditProvider(EditDataProvider* provider) override;
    bool button1Pressed(int modifierKey) override;
    void switchOffEditMode() override;

    void adjusterChanged(Adjuster* a, double newval) override;

// implement these to load / save ttp profiles
    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};
