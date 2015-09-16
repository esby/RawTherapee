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
#ifndef _THEMETOOLFAVORITE_COLOR_CHOOSER_H_
#define _THEMETOOLFAVORITE_COLOR_CHOOSER_H_

#include <memory>
#include <gtkmm.h>
#include "toolpanel.h"
#include "../rtengine/rawimage.h"
#include "guiutils.h"


class TTFavoriteColorChooser : public ToolParamBlock, public FoldableToolPanel {

protected:

    Gtk::HBox* themeBox1;
    Gtk::HBox* themeBox2;

    Gtk::Label* lbFavorite;
    Gtk::ColorButton* cbFavorite; 
    Gtk::Label* lbTrash;
    Gtk::ColorButton* cbTrash;

public:

    TTFavoriteColorChooser();

    void read           (const rtengine::procparams::ProcParams* pp, const ParamsEdited* pedited=NULL);
    void write          (rtengine::procparams::ProcParams* pp, ParamsEdited* pedited=NULL);
    void deploy();
   
   // void on_toggle_button();
   void enabledChanged  ();

    void colorButton(Gtk::ToggleButton* button, Gdk::Color color, bool deactivate=false);
    void colorFavorite(ToolPanel* panel, bool deactivate=false);
    void colorTrash(ToolPanel* panel, bool deactivate=false);

    bool canBeEnabled() {return true;}

};

#endif