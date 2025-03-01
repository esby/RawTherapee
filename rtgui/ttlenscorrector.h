/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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
#ifndef _THEMETOOL_LENS_CORRECTOR_H_
#define _THEMETOOL_LENS_CORRECTOR_H_

#include <memory>
#include <gtkmm.h>
#include "toolpanel.h"
#include "../rtengine/rawimage.h"
#include "guiutils.h"
#include "distortion.h"


class TTLensCorrector : public ToolParamBlock, public FoldableToolPanel {

protected:

    Gtk::HBox* themeBox1;

    Gtk::Label* lbDistortionCorrect;
    Gtk::CheckButton* cbDistortionCorrect; 

    Distortion* distortion;

    std::map<int, double> vid;
    bool value_not_found;
    std::string lenspath;

public:

    TTLensCorrector();
    void deploy();
    void deployLate();
    void react(FakeProcEvent ev);
    void enabledChanged  ();

    bool canBeEnabled() {return true;}
    void load_data();

// implement these to load / save ttp profiles
    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};

#endif
