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

#include <gtkmm.h>
#include "esbysettings.h"

// tools tab of the preferences dialog (esby fork): tabs hidden at startup.
// a member of Preferences (Preferences::esbyPanel), the code was moved from preferences.cc/h.
class EsbyPreferencesPanel
{
public:
    Gtk::Widget* getTTPanel ();
    void fillPreferences(const EsbySettings& settings);
    void storePreferences(EsbySettings& settings);

private:
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
    Gtk::HBox* themeBox11;

    Gtk::Label* lbHideFavorite;
    Gtk::CheckButton* cbHideFavorite;
    Gtk::Label* lbHideExposure;
    Gtk::CheckButton* cbHideExposure;
    Gtk::Label* lbHideDetails;
    Gtk::CheckButton* cbHideDetails;
    Gtk::Label* lbHideColor;
    Gtk::CheckButton* cbHideColor;
    Gtk::Label* lbHideAdvanced;
    Gtk::CheckButton* cbHideAdvanced;
    Gtk::Label* lbHideLocal;
    Gtk::CheckButton* cbHideLocal;
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
};
