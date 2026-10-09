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

// series exposure. A reference image is set for a series (its numbered folder) or for the parent
// folder (inherited by the series below it): the shared variable exposure.reference (esby server).
// Each other image follows a reference (the nearest one by default, the one of the series, the one
// of the parent, or none: not applied), and has a gap with it (delta, EV), measured with the pipette
// (brightness of a reference area) or by the external measuring service (face).
//
//   compensation = compensation of the reference + delta + offset of the series
//
// The offset (shared variable exposure.offset) is a rendering preference of the series. The state of
// each image (followed reference, delta, last compensation applied) is kept by the server, so that
// every instance, and the command line (esbywb.py exposure-apply), computes the same value.
// An image changed by hand stops following its reference (not applied).
class TTSeriesExposure : public ToolParamBlock, public FoldableToolPanel, public EditSubscriber, public AdjusterListener
{
protected:
    // a reference, as kept in exposure.reference (JSON): raw file, compensation without the offset,
    // point of the face (service), brightness target without the offset (pipette)
    struct Reference
    {
        bool valid = false;
        Glib::ustring raw;
        double comp = 0.0;
        bool hasPoint = false;
        double px = 0.0, py = 0.0;
        bool hasTarget = false;
        double target = 0.0;
        Glib::ustring origin; // folder where it is set
        static Reference parse(const Glib::ustring& text, const Glib::ustring& origin);
        Glib::ustring serialize() const;
    };

    Gtk::ToggleButton* tbPipette;
    MyComboBoxText* cbLevel;       // where the reference of the next button / ctrl + click is set
    Gtk::Button* btReference;      // the current image becomes the reference (with the service)
    Gtk::Button* btNoReference;    // the current image is not a reference anymore
    MyComboBoxText* cbFollow;      // reference followed by the current image
    Gtk::Button* btMeasure;        // measure the current image again (service)
    Gtk::Button* btApply;          // apply the suggested gap
    Adjuster* adjOffset;           // offset of the series (EV): shared variable exposure.offset
    Gtk::Label* lbInfo;

    std::unique_ptr<EsbyHttpClient> service;
    ToneCurve* tonecurve;

    // series of the current image
    Glib::ustring sequenceKey;     // its numbered folder
    Glib::ustring sequenceText;
    bool sequenceReady;

    // state of the current image (server), and the references it can follow
    Glib::ustring stateFile;       // image the state below belongs to
    bool stateLoaded;
    Glib::ustring follow;          // nearest, series, parent, none
    bool hasDelta;
    double delta;
    bool hasLastComp;
    double lastComp;               // compensation last applied by the tool (any instance)
    bool hasShownComp;
    double shownComp;              // compensation set by this instance on the current image
    Reference refSeries, refParent;
    int refsPending;
    int generation;                // answers for a previous image are ignored
    int refGeneration;             // answers of a previous load of the references are ignored
    bool hasSuggestion;
    double suggestedDelta;
    bool recomputePending;         // a change came while the window was not active
    bool sharedConnected;
    bool focusConnected;
    bool updatingWidgets;

    Glib::ustring originalFile();  // ESBY_ORIGIN, or the current file
    static Glib::ustring sequenceFolder(const Glib::ustring& file);
    void startSequence();
    void publishSequence();

    bool measure(double& luminance);
    double currentExpComp();
    void setExpComp(double value);
    double seriesOffset();
    bool exposureIsAutomatic();
    static bool readAfPoint(const Glib::ustring& file, double& x, double& y);

    void loadImageState();
    void loadReferences();
    const Reference& followedReference();
    Glib::ustring sessionOf(const Reference& ref);
    void recompute(bool fromEvent);
    void setFollow(const Glib::ustring& f, bool save);
    void storeDelta(double d);
    void saveApplied(double comp);
    void setReferenceHere(bool hasPoint, double px, double py, bool hasTarget, double target);
    void publishReference(const Reference& ref, const Glib::ustring& folder);
    void removeReference();
    void measureCurrent(bool manual, bool retried = false);
    void setSuggestion(double d, const Glib::ustring& text);
    void sharedChanged();
    void offsetAdjusted();
    void setInfo(const Glib::ustring& action);

public:
    TTSeriesExposure();
    ~TTSeriesExposure() override;

    void deploy();
    void deployLate();
    void react(FakeProcEvent ev);
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
