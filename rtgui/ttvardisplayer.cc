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
#include "ttvardisplayer.h"
#include "options.h"
#include "guiutils.h"
#include "rtimage.h"
#include "rtdef.h"
#include "guiutils.h"
#include <fstream>

using namespace rtengine;
using namespace rtengine::procparams;

TTVarDisplayer::TTVarDisplayer () : FoldableToolPanel(this,"ttvardisplayer",M("TT_VAR_DISPLAYER_LABEL"),false,false)
{
        //todo: find a f!cking way to set a minimum size for a tool
	hboxr = Gtk::manage(new Gtk::HBox());
	pack_start(*hboxr, Gtk::PACK_SHRINK, 0);

	vbox1 = Gtk::manage(new Gtk::VBox());
	vbox1->set_spacing(4);
        Gtk::Label* labelVar = Gtk::manage(new Gtk::Label (M("TT_VAR_DISPLAYER_LABEL_VARIABLES")));
        vbox1->pack_start(*labelVar, Gtk::PACK_SHRINK, 0);

	hboxr->pack_start(*vbox1, Gtk::PACK_EXPAND_WIDGET ,0);
}

void TTVarDisplayer::deploy()
{
	FoldableToolPanel::deploy();
        env->registerPriority(getToolName());

}

void TTVarDisplayer::deployLate()
{
	FoldableToolPanel::deployLate();
}

void TTVarDisplayer::react(FakeProcEvent ev)
{
	if (ev == FakeEvExifTransmitted)
	{
		// we first create labels & entries to display the env variables
		for (size_t i=varBox.size(); i<env->countVar(); i++)
		{
			Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
			hbox->set_spacing(4);

			Gtk::Label* lbl = Gtk::manage(new Gtk::Label ());
			lbl->set_width_chars(6);
			hbox->pack_start(*lbl,  Gtk::PACK_SHRINK,0);

			Gtk::Entry* entry = Gtk::manage (new Gtk::Entry ());
			entry->set_width_chars(25);
			hbox->pack_start(*entry, Gtk::PACK_SHRINK,0);

			vbox1->pack_start(*hbox, Gtk::PACK_SHRINK,0);

			varBox.push_back(hbox);
			varLabel.push_back(lbl);
			varEntry.push_back(entry);
		}

		// we feed the new values
		printf("TTVarDisplayer React \n");
		for (size_t i=0; i<env->countVar(); i++)
		{
			RtVariable* d = env->getVariable(i);
			if (d != nullptr)
			{
				varLabel[i]->set_text(d->getName());
				varEntry[i]->set_text(d->toString());
				printf("variable : %s ", d->getName().c_str());
				printf("value: %s \n", d->toString().c_str());
			}
		}


		// we display the entries
		getExpander()->show_all();

	}
}

//void TTUDLRHider::on_toggle_button()
void TTVarDisplayer::enabledChanged  () 
{
   
}

