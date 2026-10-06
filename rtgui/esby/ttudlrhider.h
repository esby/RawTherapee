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
#ifndef _THEMETOOLFAVORITE_UDLR_HIDER_H_
#define _THEMETOOLFAVORITE_UDLR_HIDER_H_

#include <memory>
#include <gtkmm.h>
#include "toolpanel.h"
#include "../rtengine/rawimage.h"
#include "guiutils.h"


class TTUDLRHider : public ToolParamBlock, public FoldableToolPanel {

protected:

    Gtk::HBox* themeBox1;
    Gtk::HBox* themeBox2;

    Gtk::Label* lbHideArrow;
    Gtk::Label* lbLockFav;

    Gtk::CheckButton* cbHideArrow;
    Gtk::CheckButton* cbLockFav;

    bool disableButtons;

public:

    TTUDLRHider();

    void deploy();
    void deployLate();
    void enabledChanged  ();

    void actOnPanel(ToolPanel* panel);
    void react(FakeProcEvent ev);


    bool canBeEnabled() {return true;}

// implement these to load / save ttp profiles
    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};

#endif
