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
#ifndef _THEMETOOL_PANEL_COLOR_CHOOSER_H_
#define _THEMETOOL_PANEL_COLOR_CHOOSER_H_

#include <memory>
#include <gtkmm.h>
#include "toolpanel.h"
#include "../rtengine/rawimage.h"
#include "guiutils.h"

class TTPanelColorChooser : public ToolParamBlock, public FoldableToolPanel {

protected:

    Gtk::HBox* themeBox1;
    Gtk::Label* lbPanel1;
    Gtk::ColorButton* cbPanel1; 

    Gtk::HBox* themeBox2;
    Gtk::Label* lbPanel2;
    Gtk::ColorButton* cbPanel2;

    Gtk::Bin* e;

public:

    TTPanelColorChooser();

    void deploy();
 
    void on_toggle_button();
    void react(FakeProcEvent ev);


    int getState(ToolPanel* panel);
    Gdk::RGBA getColor(ToolPanel* panel, int state);

    void colorPanel(ToolPanel* panel, bool deactivate=false);
    void pangoPanel(ToolPanel* panel, bool deactivate=false);

    void colorer(ToolPanel* panel);
    void pangorer(ToolPanel* panel);
 
    bool canBeEnabled() {return true;}
    void test(); 

// implement these to load / save ttp profiles
    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};

#endif
