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
#include "ttudlrhider.h"
#include "options.h"
#include "guiutils.h"
#include "../rtengine/safegtk.h"
#include "rtimage.h"
#include <sstream>

using namespace rtengine;
using namespace rtengine::procparams;

TTUDLRHider::TTUDLRHider () : FoldableToolPanel(this,"ttudlrhider",M("TP_UDLR_HIDER_LABEL"),false,true)
{
  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);
  themeBox2 = Gtk::manage(new Gtk::HBox());
  themeBox2->set_spacing(4);

  lbHideArrow = Gtk::manage(new Gtk::Label(M("TT_UDLR_HIDE_ARROWS")));
  cbHideArrow = Gtk::manage(new Gtk::CheckButton());

  lbLockFav = Gtk::manage(new Gtk::Label(M("TT_UDLR_LOCK_FAV")));
  cbLockFav = Gtk::manage(new Gtk::CheckButton());

  themeBox1->pack_start(*lbHideArrow, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_end(*cbHideArrow, Gtk::PACK_SHRINK, 0);

  themeBox2->pack_start(*lbLockFav, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_end(*cbLockFav, Gtk::PACK_SHRINK, 0);


  pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox2, Gtk::PACK_SHRINK, 0);

}

void TTUDLRHider::deploy()
{
   FoldableToolPanel::deploy();
   for (size_t i=0; i< env->countPanel() ; i++)
   { 
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {
        printf("connecting to %s \n", p->getToolName().c_str());
        actOnPanel(p, true);
        actOnPanel(p, true);
      }
   }

  // button enable / disable
   getExpander()->signal_enabled_toggled().connect(sigc::mem_fun(this, &TTUDLRHider::enabledChanged));
   cbLockFav->signal_clicked().connect( sigc::mem_fun(this, &TTUDLRHider::enabledChanged));
   cbHideArrow->signal_clicked().connect( sigc::mem_fun(this, &TTUDLRHider::enabledChanged));

}

void TTUDLRHider::read(const rtengine::procparams::ProcParams* pp, const ParamsEdited* pedited)
{

}

void TTUDLRHider::write( rtengine::procparams::ProcParams* pp, ParamsEdited* pedited)
{

}

//void TTUDLRHider::on_toggle_button()
void TTUDLRHider::enabledChanged  () 
{  
   //  if (getEnabledButton()->get_active()) 
   if (getExpander()->getEnabled())
   for (size_t i=0; i< env->countPanel() ; i++)
   {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {
        actOnPanel(p);
      }
  }
  else
  for (size_t i=0; i< env->countPanel() ; i++)
  {
     FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {
        actOnPanel(p, true);
      }
  }
}

void  TTUDLRHider::actOnPanel(ToolPanel* panel, bool deactivate)
{  
  if (panel != NULL)
  {
    //if((!deactivate) && (getEnabledButton()->get_active()))
    if((!deactivate) && (getExpander()->getEnabled()))
    {
      if (!deactivate && cbHideArrow->get_active())
      {
        panel->getMoveRButton()->hide();
        panel->getMoveLButton()->hide();
        panel->getMoveUButton()->hide();
        panel->getMoveDButton()->hide();
      }
      else
      {
        panel->getMoveRButton()->show();
        panel->getMoveLButton()->show();
        panel->getMoveUButton()->show();
        panel->getMoveDButton()->show();
      }

      if (!deactivate && cbLockFav->get_active())
      { 
        panel->getFavoriteButton()->set_sensitive(false);
        panel->getTrashButton()->set_sensitive(false);
      }
      else
      {
        panel->getFavoriteButton()->set_sensitive(true);
        panel->getTrashButton()->set_sensitive(true);
      }    
    }   
  }
}


