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
#ifndef _THEMETOOL_VAR_DISPLAYER_H_
#define _THEMETOOL_VAR_DISPLAYER_H_

#include <memory>
#include <gtkmm.h>
#include "toolpanel.h"
#include "../rtengine/rawimage.h"
#include "guiutils.h"


class TTVarDisplayer : public ToolParamBlock, public FoldableToolPanel {

	protected:
		Gtk::HBox* hboxr;
		Gtk::Button* copyButton;
		Gtk::VBox* vbox1;
		Gtk::VBox* vbox2;

		// displaying variables
		std::vector<Gtk::HBox*>  varBox;
		std::vector<Gtk::Entry*> varLabel;
		std::vector<Gtk::Entry*> varEntry;

		// shared variables (esby server)
		struct SharedRow
		{
			Gtk::HBox* box;
			Gtk::Label* label;
			Gtk::Entry* value;
			Gtk::Label* originLabel;
			Gtk::Button* unset;
			Glib::ustring name;
			Glib::ustring origin; // key where the value is set (folder, folder#sequence, *)
		};
		Gtk::Label* lbShared;
		Gtk::VBox* sharedBox;
		std::vector<SharedRow> sharedRows;
		Gtk::HBox* addBox;
		Gtk::Entry* addName;
		Gtk::Entry* addValue;
		MyComboBoxText* addWhere;
		bool sharedConnected;

		void refreshVariables();
		void refreshShared();
		void createSharedRow();
		void sharedValueChanged(size_t index);


	public:

		TTVarDisplayer();
		~TTVarDisplayer(){};

		void deploy();
		void deployLate();
		void react(FakeProcEvent ev);
		void enabledChanged  ();
		void copy_clicked ();
		void add_clicked ();

    bool canBeEnabled() {return true;}
};

#endif
