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
#ifndef _THEMETOOL_TWEAKER_H_
#define _THEMETOOL_TWEAKER_H_

#include <memory>
#include <gtkmm.h>
#include "toolpanel.h"
#include "../rtengine/rawimage.h"
#include "guiutils.h"
#include "coarsepanel.h"
#include "distortion.h"
#include "whitebalance.h"
#include "rotate.h"


class TTTweaker : public ToolParamBlock, public FoldableToolPanel {

protected:

    Gtk::HBox* themeBox1;
    Gtk::HBox* themeBox2;
    Gtk::HBox* themeBox3;
    Gtk::HBox* themeBox4;
    Gtk::HBox* themeBox5;
    Gtk::HBox* themeBox6;
    Gtk::HBox* themeBox7;

    Gtk::Label* lbAutoDistortionCorrect;
    Gtk::CheckButton* cbAutoDistortionCorrect; 

    Gtk::Label* lbCloseAfterSave;
    Gtk::CheckButton* cbCloseAfterSave;

    Gtk::Label* lbReduceAfterSave;
    Gtk::CheckButton* cbReduceAfterSave;

    Gtk::Label* lbResetWBForRt4Profiles;
    Gtk::CheckButton* cbResetWBForRt4Profiles;

    Gtk::Label* lbAutoRotateCorrect;
    Gtk::CheckButton* cbAutoRotateCorrect;

    Gtk::Label* lbToolNameAsToolTip;
    Gtk::CheckButton* cbToolNameAsToolTip;

    Gtk::Label* lbToolNameUntranslated;
    Gtk::CheckButton* cbToolNameUntranslated;

    CoarsePanel* coarse;
    Rotate* rotate;
    WhiteBalance* whitebalance;
    Distortion* distortion;

public:

    TTTweaker();
    void deploy();
    void deployLate();
    void react(FakeProcEvent ev);
    void enabledChanged  ();

    bool canBeEnabled() {return true;}

// implement these to load / save ttp profiles
    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};

#endif
