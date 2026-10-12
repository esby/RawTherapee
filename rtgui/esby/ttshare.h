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
#include <set>
#include <string>
#include <vector>
#include <gtkmm.h>
#include "toolpanel.h"
#include "guiutils.h"
#include "esbywbclient.h"

// shared changes: a change made in an image is shared with the images of its series (its numbered
// folder): the other instances showing an image of the series, and the images opened later.
//
// The parameters are compared as the keys of the pp3 ("Section/Key"), for the shared sections only:
// - a number (exposure compensation, contrast, black...) is shared as a shift: each image keeps its
//   own value, plus the shifts of the series;
// - anything else (curve, choice, check box) is shared as a value, which replaces the previous one.
// The server keeps, per series, the sum of the shifts of each key (or its last value and version), and
// per image, what it has already received: an image opened later gets what it missed.
// With "Share my changes" off, a change stays in the image (its own gap); changes are still received.
// A change made by a tool for this image only (TTSeriesExposure) is never shared.
class TTShare : public ToolParamBlock, public FoldableToolPanel
{
protected:
    Gtk::CheckButton* cbShare;
    std::map<std::string, Gtk::CheckButton*> cbSections;
    Gtk::Button* btToParent;
    Gtk::Button* btFromParent;
    Gtk::Label* lbInfo;

    std::map<std::string, std::string> snapshot; // shared keys of the parameters before the next change
    bool hasSnapshot;

    // state of the current image (server): sums of the shifts and versions already received
    Glib::ustring file;
    Glib::ustring folder;
    bool stateLoaded;
    int generation;
    std::map<std::string, double> appliedCum;
    std::map<std::string, int> appliedVersion;

    bool connected;
    bool pending;          // changes received while the window was not active
    bool focusConnected;

    std::set<std::string> sharedSections();
    bool isShared(const std::string& key);
    std::map<std::string, std::string> extract(const rtengine::procparams::ProcParams& params);
    static Glib::ustring tempFile(const char* name);

    void connectSignals();
    void paramsChanged(const rtengine::procparams::ProcParams& params, const rtengine::ProcEvent& ev, bool fromPanel);
    void loadState();
    void catchUp(const std::vector<EsbyShareEntry>& entries, bool fromEvent);
    void saveApplied(const std::vector<std::string>& keys);
    void copyWithParent(bool toParent);
    void setInfo(const Glib::ustring& text);

public:
    TTShare();
    ~TTShare() override;

    void deploy();
    void deployLate();
    void react(FakeProcEvent ev);
    void enabledChanged() override;
    bool canBeEnabled() {return true;}

    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};
