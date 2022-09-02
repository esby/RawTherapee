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
#ifndef _THEMETOOL_SAVER_H_
#define _THEMETOOL_SAVER_H_

#include <memory>
#include <gtkmm.h>
#include "toolpanel.h"
#include "../rtengine/rawimage.h"
#include "guiutils.h"
#include "pathutils.h"


class TTSaver : public ToolParamBlock, public FoldableToolPanel {

protected:

    Gtk::HBox* themeBox1;
    Gtk::HBox* themeBox2;
    Gtk::HBox* themeBox3;

    Gtk::Label* themeLabel;
    Gtk::Button* buttonLoad;
    Gtk::Button* buttonSave;
    sigc::connection s;
    MyComboBoxText* profilbox;

    Gtk::Label* lbAutoloadSettings;
    Gtk::CheckButton* cbAutoloadSettings;

    Gtk::Label* lbAutoloadSettingsLine;

    std::vector<Glib::ustring> entries;


protected:

    Glib::ustring getInternalSelected();

public:

    TTSaver ();
    ~TTSaver() override;
    void themeSplitter(std::ifstream& myfile);
    void load_ttp_profile(Glib::ustring filename);
    void save_ttp_profile(Glib::ustring filename);
    void resetFavoriteAndTrashState();
    void save_clicked (GdkEventButton* event);
    void autoload_clicked (GdkEventButton* event);
   
    void parseProfileFolder();

    void profileBoxChanged();
    void deploy();



    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();
};

#endif
