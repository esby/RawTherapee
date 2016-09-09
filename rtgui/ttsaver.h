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


class TTSaver : public ToolParamBlock, public FoldableToolPanel {

protected:

    Gtk::HBox* themeBox;
    Gtk::Label* themeLabel;
    Gtk::Button* buttonLoad;
    Gtk::Button* buttonReset;
    Gtk::Button* buttonSave;
    sigc::connection s;
    MyComboBoxText* profilbox;

public:

    TTSaver ();

    void themeImport(std::ifstream& myfile);
    Glib::ustring themeExport();

    void themeLoad();
    void themeReset();
    void themeSave();
    void resetFavoriteAndTrashState();
    void test(Glib::ustring name);
    void parseProfileFolder();
};

#endif
