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
#ifndef _THEMETOOL_ISO_PROFILER_H_
#define _THEMETOOL_ISO_PROFILER_H_

#include <memory>
#include <gtkmm.h>
#include "toolpanel.h"
#include "../rtengine/rawimage.h"
#include "guiutils.h"


class TTIsoProfiler : public ToolParamBlock, public FoldableToolPanel {

protected:

    Gtk::HBox* themeBox1;
    Gtk::HBox* themeBox2;
    Gtk::VBox* themeBox3;

    Gtk::Label* themeIsoLabel;
    Gtk::Label* themeProfilePickerLabel;
    Gtk::Button* buttonLoad;
    Gtk::Button* buttonSave;
    sigc::connection s;

    Gtk::Entry* entry;
    MyComboBoxText* profilbox;

    Gtk::Label* lbAutoloadSettings;
    Gtk::CheckButton* cbAutoloadSettings;

    Gtk::Label* lbAutoloadSettingsLine;

    std::vector<Glib::ustring> paths;
    std::vector<Glib::ustring> names;

    std::vector<Glib::ustring> listIsos;
    std::vector<Glib::ustring> listNames;
    std::vector<Glib::ustring> listPaths;

    std::vector<Gtk::Entry*> entryNames;
    std::vector<Gtk::Entry*> entryIsos;
    std::vector<Gtk::Button*> entryDelButtons;


    bool donotreacttoprofileload;

//    std::vector<vector<Glib::ustring>> isoEntries;
//    std::vector<Glib::ustring> boxEntries;


protected:

    Glib::ustring getInternalSelected();

public:

    TTIsoProfiler ();
    void load_profile(Glib::ustring filename);
    void save_profile(Glib::ustring isolimit, Glib::ustring name, Glib::ustring filename);
    void affect_profiles();
    void save_clicked (GdkEventButton* event);
    void autoload_clicked (GdkEventButton* event);
    void delete_clicked (int pos);


    void react(FakeProcEvent ev);

   
    void parseProfileFolder();

    void profileBoxChanged();
    void deploy();

    void sortVectorsByIsos();



    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};

#endif
