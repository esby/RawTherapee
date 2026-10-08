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
#include <mutex>
#include <vector>
#include <gtkmm.h>
#include "toolpanel.h"
#include "guiutils.h"
#include "editcallbacks.h"
#include "widgets/basic/adjuster.h"
#include "esbyhttpclient.h"

class ToneCurve;

// series exposure: the brightness of a reference area (a face) is learned on a reference image of a
// sequence (ctrl + click with the pipette), then a click on the same area of the other images of the
// sequence sets their exposure compensation to reach it.
//
// sequence: the numbered folder of the image (the folder holding pp/, pp/dpp/), split when there is
// more than a given time between two photos, if its fields.conf credits one model only.
// The targets are kept in a local file (one RawTherapee instance per image with rt_queue).
class TTSeriesExposure : public ToolParamBlock, public FoldableToolPanel, public EditSubscriber, public AdjusterListener
{
protected:
    Gtk::ToggleButton* tbPipette;
    Adjuster* adjGap;            // time between two photos starting a new sequence (minutes)
    Adjuster* adjOffset;         // offset of the sequence (EV): shared variable exposure.offset
    Gtk::Button* btToggle;       // back to the compensation before an offset change, and again
    Gtk::Label* lbInfo;

    // external measuring service (face detection and recognition, measure in the raw data)
    std::unique_ptr<EsbyHttpClient> service;
    Gtk::Button* btReference;    // the current image becomes the reference (face near the AF point)
    Gtk::Button* btMeasure;      // measure the current image again
    Gtk::Button* btApply;        // apply the suggested compensation
    double suggestedComp;
    bool hasSuggestion;

    ToneCurve* tonecurve;

    // sequence of the current image
    Glib::ustring sequenceKey;   // folder, or folder#start time of the part
    Glib::ustring sequenceText;  // description shown in the status line
    bool sequenceReady;

    // offset of the sequence (shared variable exposure.offset, nearest of sequence, folders, global):
    // a rendering preference added to the measured compensation, kept apart from the measures
    bool sharedConnected;
    Glib::ustring offsetFile;    // image the old/new values below belong to
    double toggleOld, toggleNew; // compensations before and after the last offset change
    bool toggleShowsNew;
    double sequenceOffset();
    void offsetChanged();
    void offsetAdjusted();

    // capture times of the photos of a folder, read in a thread (the folder may be on a network drive)
    struct Scan
    {
        std::mutex mutex;
        bool alive = true;
        bool done = false;
        Glib::ustring folder;
        std::map<Glib::ustring, double> times; // file name -> capture time (seconds)
    };
    std::shared_ptr<Scan> scan;
    Glib::Dispatcher scanDone;

    Glib::ustring originalFile();                 // ESBY_ORIGIN, or the current file
    static Glib::ustring sequenceFolder(const Glib::ustring& file);
    static int countModels(const Glib::ustring& folder);
    static double captureTime(const Glib::ustring& file);
    void startSequence();
    void publishSequence();
    void sequenceScanned();

    bool measure(double& luminance);
    double currentExpComp();
    void setExpComp(double value);
    static Glib::ustring targetsFile();
    bool readTarget(double& luminance, Glib::ustring& reference);
    void writeTarget(double luminance, const Glib::ustring& reference);
    void setInfo(const Glib::ustring& action);

    // service: sequence reference (kept with the target: raw, point, AF point, compensation)
    struct ServiceRef
    {
        bool valid = false;
        Glib::ustring raw;
        bool hasPoint = false;
        double px = 0.0, py = 0.0;
        double comp = 0.0;
    };
    bool readServiceRef(ServiceRef& ref);
    void writeServiceRef(const ServiceRef& ref);
    // compensation applied by the tool to a file, and the offset of the sequence it included
    bool readApplied(const Glib::ustring& file, double& comp, double* offset = nullptr);
    void writeApplied(const Glib::ustring& file, double comp, double offset);
    static bool readAfPoint(const Glib::ustring& file, double& x, double& y);
    void setReference(bool hasPoint, double px, double py);
    void measureCurrent(bool manual, bool retried = false);
    void setSuggestion(double comp, const Glib::ustring& text);
    bool exposureIsAutomatic();

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
