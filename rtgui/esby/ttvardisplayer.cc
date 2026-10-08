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
#include "esbyoptions.h"
#include "ttlog.h"
#include "options.h"
#include "guiutils.h"
#include "rtimage.h"
#include "rtdef.h"
#include "guiutils.h"
#include <fstream>
#include <regex>

#include "ttudlrhider.h"
#include "esbysharedvars.h"
#include "variable.h"

using namespace rtengine;
using namespace rtengine::procparams;

TTVarDisplayer::TTVarDisplayer () : FoldableToolPanel(this,"ttvardisplayer",M("TT_VAR_DISPLAYER_LABEL"),false,false)
{
        //todo: find a f!cking way to set a minimum size for a tool
	hboxr = Gtk::manage(new Gtk::HBox());

	vbox1 = Gtk::manage(new Gtk::VBox());
	vbox1->set_spacing(1);
        Gtk::Label* labelVar = Gtk::manage(new Gtk::Label (M("TT_VAR_DISPLAYER_LABEL_VARIABLES")));
        vbox1->pack_start(*labelVar, Gtk::PACK_SHRINK, 0);

	hboxr->pack_start(*vbox1, Gtk::PACK_EXPAND_WIDGET ,0);

	// copies every transmitted variable to the clipboard
	copyButton = Gtk::manage(new Gtk::Button(M("TT_VAR_DISPLAYER_COPY")));
	copyButton->set_image(*Gtk::manage(new RTImage("copy")));
	copyButton->set_always_show_image(true);
	copyButton->set_tooltip_text(M("TT_VAR_DISPLAYER_COPY_TOOLTIP"));
	copyButton->signal_clicked().connect(sigc::mem_fun(*this, &TTVarDisplayer::copy_clicked));
	pack_start(*copyButton, Gtk::PACK_SHRINK, 0);

        pack_start(*hboxr, Gtk::PACK_SHRINK, 0);

	// shared variables (esby server): the ones applying to the image, and a row to add one
	sharedConnected = false;
	pack_start(*Gtk::manage(new Gtk::HSeparator()), Gtk::PACK_SHRINK, 2);
	lbShared = Gtk::manage(new Gtk::Label(M("TT_VAR_DISPLAYER_SHARED_UNAVAILABLE")));
	lbShared->set_xalign(0.0);
	lbShared->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
	pack_start(*lbShared, Gtk::PACK_SHRINK, 0);
	sharedBox = Gtk::manage(new Gtk::VBox());
	sharedBox->set_spacing(1);
	pack_start(*sharedBox, Gtk::PACK_SHRINK, 0);

	addBox = Gtk::manage(new Gtk::HBox());
	addBox->set_spacing(2);
	addName = Gtk::manage(new Gtk::Entry());
	addName->set_width_chars(12);
	addName->set_placeholder_text(M("TT_VAR_DISPLAYER_SHARED_NAME"));
	addBox->pack_start(*addName, Gtk::PACK_SHRINK, 0);
	addValue = Gtk::manage(new Gtk::Entry());
	addValue->set_width_chars(6);
	addValue->set_placeholder_text(M("TT_VAR_DISPLAYER_SHARED_VALUE"));
	addBox->pack_start(*addValue, Gtk::PACK_SHRINK, 0);
	addWhere = Gtk::manage(new MyComboBoxText());
	addWhere->append(M("TT_VAR_DISPLAYER_SHARED_SEQUENCE"));
	addWhere->append(M("TT_VAR_DISPLAYER_SHARED_FOLDER"));
	addWhere->append(M("TT_VAR_DISPLAYER_SHARED_PARENT"));
	addWhere->append(M("TT_VAR_DISPLAYER_SHARED_GLOBAL"));
	addWhere->set_active(0);
	addBox->pack_start(*addWhere, Gtk::PACK_EXPAND_WIDGET, 0);
	Gtk::Button* addButton = Gtk::manage(new Gtk::Button());
	addButton->set_image(*Gtk::manage(new RTImage("add-small")));
	addButton->set_tooltip_text(M("TT_VAR_DISPLAYER_SHARED_ADD_TOOLTIP"));
	addButton->signal_clicked().connect(sigc::mem_fun(*this, &TTVarDisplayer::add_clicked));
	addValue->signal_activate().connect(sigc::mem_fun(*this, &TTVarDisplayer::add_clicked));
	addBox->pack_start(*addButton, Gtk::PACK_SHRINK, 0);
	addBox->set_sensitive(false);
	pack_start(*addBox, Gtk::PACK_SHRINK, 0);
}

void TTVarDisplayer::copy_clicked ()
{
	// one "name = value" line per variable, read from the environment at click time.
	// empty variables are skipped (ex: exif values emptied when the previous image was replaced).
	Glib::ustring text;
	int count = 0;

	for (size_t i=0; i<env->countVar(); i++)
	{
		RtVariable* d = env->getVariable(i);
		if (d == nullptr)
			continue;

		Glib::ustring value = d->toString();
		if (value.empty())
			continue;

		text += d->getName() + " = " + value + "\n";
		count++;
	}

	Gtk::Clipboard::get()->set_text(text);

	if (esbyOptions().rtSettings.verbose)
		printf("TTVarDisplayer: %i variables copied to the clipboard\n", count);
}

void TTVarDisplayer::deploy()
{
	FoldableToolPanel::deploy();

}

void TTVarDisplayer::deployLate()
{
	FoldableToolPanel::deployLate();
}

void TTVarDisplayer::react(FakeProcEvent ev)
{
	if (ev == FakeEvExifTransmitted)
	{
		// the shared variables of the new image are loaded asynchronously: refreshed again when they arrive
		if (!sharedConnected)
		{
			env->sharedVariables()->signalChanged().connect(sigc::mem_fun(*this, &TTVarDisplayer::refreshVariables));
			sharedConnected = true;
		}
		env->sharedVariables()->setImage(EsbySharedVariables::originalFile(env));
		refreshVariables();

		// we display the entries
                // todo the current code shows arrows while they should be not displayed
		// getExpander()->show_all();

// debugging code if needed
/*
   for (size_t i=0; i< env->countPanel() ; i++)
   {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {  
        if (p->getToolName() == "rotate" )
        {
          printf("DEBUG panel.name=%s \n",p->getToolName().c_str());
          printf("DEBUG expander enabled=%i \n",p->getExpander()->getEnabled());
        }

      if  (p->getToolName() == "ttudlrhider" )
        {
          printf("DEBUG2 panel.name=%s \n",p->getToolName().c_str());
          printf("DEBUG2 expander enabled=%i \n",p->getExpander()->getEnabled());
          printf("DEBUG cbhideArrow=%i \n",static_cast<TTUDLRHider*> (p)->cbHideArrow->get_active());
          printf("DEBUG cbLockFav=%i \n", static_cast<TTUDLRHider*> (p)->cbLockFav->get_active());
//      static_cast<TTUDLRHider*> (p)->enabledChanged();
        }

      }
    }
*/
}

}

// internal and exif variables (one row per variable of the environment), then the shared variables
void TTVarDisplayer::refreshVariables()
{
	// we first create labels & entries to display the env variables
	for (size_t i=varBox.size(); i<env->countVar(); i++)
	{
		Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
		hbox->set_spacing(1);

		Gtk::Entry* lbl = Gtk::manage(new Gtk::Entry ());
		lbl->set_width_chars(20);
		hbox->pack_start(*lbl,  Gtk::PACK_SHRINK,0);

		Gtk::Entry* entry = Gtk::manage (new Gtk::Entry ());
		entry->set_width_chars(8);
		hbox->pack_start(*entry, Gtk::PACK_SHRINK,0);

		vbox1->pack_start(*hbox, Gtk::PACK_SHRINK,0);
                        vbox1->show_all();


		varBox.push_back(hbox);
		varLabel.push_back(lbl);
		varEntry.push_back(entry);
	}

	// we feed the new values
	TT_LOG("TTVarDisplayer React \n");
	for (size_t i=0; i<env->countVar(); i++)
	{
		RtVariable* d = env->getVariable(i);
		// the shared variables have their own part, below
		bool shared = (d != nullptr) && (d->getScope() != RtVariableScope::Internal) && (d->getScope() != RtVariableScope::Exif);
		varBox[i]->set_visible(!shared);
		if ((d != nullptr) && !shared)
		{
                                std::string name = d->getName().c_str();
                                name = std::regex_replace(name, std::regex("rti:Exif:"), "exif:");
			varLabel[i]->set_text(name);
			varEntry[i]->set_text(d->toString());
                                if (false) // todo remove this horrible debug switch
                                {
   				  printf("variable : %s ", name.c_str());
  				  printf("value: %s \n", d->toString().c_str());
                                }
		}
	}


	refreshShared();
}

// one row per shared variable: name, value (Enter to change it where it is set), origin, unset button
void TTVarDisplayer::refreshShared()
{
	EsbySharedVariables* shared = env->sharedVariables();
	Glib::ustring where = shared->getFolder();
	if (!shared->getSequence().empty())
		where += "  #" + shared->getSequence();
	lbShared->set_text(shared->isConnected()
		? Glib::ustring::compose(M("TT_VAR_DISPLAYER_SHARED_FOR"), where.empty() ? "-" : where)
		: M("TT_VAR_DISPLAYER_SHARED_UNAVAILABLE"));
	addBox->set_sensitive(shared->isConnected());

	size_t count = 0;
	for (size_t i=0; i<env->countVar(); i++)
	{
		RtVariable* d = env->getVariable(i);
		if ((d == nullptr) || !d->isDefined() || (d->getScope() == RtVariableScope::Internal) || (d->getScope() == RtVariableScope::Exif))
			continue;
		if (count == sharedRows.size())
			createSharedRow();
		SharedRow& row = sharedRows[count];
		row.name = d->getName();
		row.origin = d->getOrigin();
		row.label->set_text(row.name);
		row.value->set_text(d->toString());
		Glib::ustring origin = (row.origin == "*") ? Glib::ustring("global") : Glib::ustring(Glib::path_get_basename(row.origin));
		row.originLabel->set_text(Glib::ustring(rtVariableScopeName(d->getScope())) + ": " + origin);
		row.originLabel->set_tooltip_text(row.origin);
		row.box->show_all();
		count++;
	}
	for (size_t i=count; i<sharedRows.size(); i++)
		sharedRows[i].box->hide();
}

void TTVarDisplayer::createSharedRow()
{
	SharedRow row;
	size_t index = sharedRows.size();
	row.box = Gtk::manage(new Gtk::HBox());
	row.box->set_spacing(2);
	row.label = Gtk::manage(new Gtk::Label());
	row.label->set_width_chars(16);
	row.label->set_xalign(0.0);
	row.box->pack_start(*row.label, Gtk::PACK_SHRINK, 0);
	row.value = Gtk::manage(new Gtk::Entry());
	row.value->set_width_chars(8);
	row.value->set_tooltip_text(M("TT_VAR_DISPLAYER_SHARED_VALUE_TOOLTIP"));
	row.value->signal_activate().connect([this, index]() { sharedValueChanged(index); });
	row.box->pack_start(*row.value, Gtk::PACK_SHRINK, 0);
	row.originLabel = Gtk::manage(new Gtk::Label());
	row.originLabel->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
	row.originLabel->set_xalign(0.0);
	row.box->pack_start(*row.originLabel, Gtk::PACK_EXPAND_WIDGET, 0);
	row.unset = Gtk::manage(new Gtk::Button());
	row.unset->set_image(*Gtk::manage(new RTImage("cancel-small")));
	row.unset->set_relief(Gtk::RELIEF_NONE);
	row.unset->set_tooltip_text(M("TT_VAR_DISPLAYER_SHARED_UNSET_TOOLTIP"));
	row.unset->signal_clicked().connect([this, index]() {
		env->sharedVariables()->unsetAt(sharedRows[index].origin, sharedRows[index].name);
	});
	row.box->pack_start(*row.unset, Gtk::PACK_SHRINK, 0);
	sharedBox->pack_start(*row.box, Gtk::PACK_SHRINK, 0);
	sharedRows.push_back(row);
}

// a new value typed for a shared variable: set where the current value comes from
void TTVarDisplayer::sharedValueChanged(size_t index)
{
	SharedRow& row = sharedRows[index];
	env->sharedVariables()->setAt(row.origin, EsbySharedVariables::parseValue(row.name, row.value->get_text()));
}

void TTVarDisplayer::add_clicked()
{
	Glib::ustring name = addName->get_text();
	if (name.empty())
		return;
	const EsbySharedVariables::Where where[] = {EsbySharedVariables::Where::Sequence, EsbySharedVariables::Where::Folder,
	                                          EsbySharedVariables::Where::Parent, EsbySharedVariables::Where::Global};
	int choice = addWhere->get_active_row_number();
	if ((choice < 0) || (choice > 3))
		choice = 0;
	env->sharedVariables()->set(where[choice], EsbySharedVariables::parseValue(name, addValue->get_text()));
	addName->set_text("");
	addValue->set_text("");
}

//void TTUDLRHider::on_toggle_button()
void TTVarDisplayer::enabledChanged  () 
{
   
}

