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
#ifndef _THEMETOOL_TAB_HIDER_H_
#define _THEMETOOL_TAB_HIDER_H_

#include <memory>
#include <gtkmm.h>
#include "toolpanel.h"
#include "../rtengine/rawimage.h"
#include "guiutils.h"


class TTTabHider : public ToolParamBlock, public FoldableToolPanel {

protected:

    Gtk::HBox* themeBox1;
    Gtk::HBox* themeBox2;
    Gtk::HBox* themeBox3;
    Gtk::HBox* themeBox4;
    Gtk::HBox* themeBox5;
    Gtk::HBox* themeBox6;
    Gtk::HBox* themeBox7;
    Gtk::HBox* themeBox8;
    Gtk::HBox* themeBox9;
    Gtk::HBox* themeBox10;

    Gtk::Label* lbHideFavorite;
    Gtk::CheckButton* cbHideFavorite;
    Gtk::Label* lbHideExposure;
    Gtk::CheckButton* cbHideExposure;
    Gtk::Label* lbHideDetails;
    Gtk::CheckButton* cbHideDetails;
    Gtk::Label* lbHideColor;
    Gtk::CheckButton* cbHideColor;
    Gtk::Label* lbHideLocal;
    Gtk::CheckButton* cbHideLocal;
    Gtk::Label* lbHideAdvanced;
    Gtk::CheckButton* cbHideAdvanced;
    Gtk::Label* lbHideTransform;
    Gtk::CheckButton* cbHideTransform;
    Gtk::Label* lbHideRaw;
    Gtk::CheckButton* cbHideRaw;
    Gtk::Label* lbHideMetadata;
    Gtk::CheckButton* cbHideMetadata;
    Gtk::Label* lbHideUseful;
    Gtk::CheckButton* cbHideUseful;
    Gtk::Label* lbHideTrash;
    Gtk::CheckButton* cbHideTrash;

    Gtk::Button* buttonSave;
    bool reacted;



public:

    TTTabHider ();
    void deploy();
    void deployLate();
    void react(FakeProcEvent ev);
    void enabled_changed ();
    void hide_favorite_clicked ();
    void hide_exposure_clicked ();
    void hide_details_clicked ();
    void hide_color_clicked ();
    void hide_advanced_clicked ();
    void hide_local_clicked ();
    void hide_transform_clicked ();
    void hide_raw_clicked ();
    void hide_metadata_clicked ();
    void hide_useful_clicked ();
    void hide_trash_clicked ();

    void save_clicked (GdkEventButton* event);

    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};

#endif
