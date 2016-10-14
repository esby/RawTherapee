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
#include "rtimage.h"
#include <fstream>

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
   /*
   for (size_t i=0; i< env->countPanel() ; i++)
   { 
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {
//        printf("connecting to %s \n", p->getToolName().c_str());
        actOnPanel(p, true);
        actOnPanel(p, true);
      }
   }*/
 //  enabledChanged();
//  enabledChanged();


  // button enable / disable
   getExpander()->signal_enabled_toggled().connect(sigc::mem_fun(this, &TTUDLRHider::enabledChanged));
   cbLockFav->signal_toggled().connect( sigc::mem_fun(this, &TTUDLRHider::enabledChanged));
   cbHideArrow->signal_toggled().connect( sigc::mem_fun(this, &TTUDLRHider::enabledChanged));

}

//void TTUDLRHider::on_toggle_button()
void TTUDLRHider::enabledChanged  () 
{
  printf("enabled=%i hidarrow=%i lockFav=%i \n", (getExpander()->getEnabled()?1:0), (cbHideArrow->get_active()?1:0), (cbLockFav->get_active()?1:0));
   
   for (size_t i=0; i< env->countPanel() ; i++)
   {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {
        actOnPanel(p);
      }
  }
  printf("enabled changed done\n");
}

void  TTUDLRHider::actOnPanel(ToolPanel* panel)
{  
  if (panel != NULL)
  {
    if (getExpander()->getEnabled())
    {
      if ( cbHideArrow->get_active())
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

      if (cbLockFav->get_active())
      {
//        panel->getFavoriteButton()->set_sensitive(false);
//        panel->getTrashButton()->set_sensitive(false);
      }
      else
      {
//        panel->getFavoriteButton()->set_sensitive(true);
//        panel->getTrashButton()->set_sensitive(true);
      }
    }
    else
    { 
      panel->getMoveRButton()->show();
      panel->getMoveLButton()->show();
      panel->getMoveUButton()->show();
      panel->getMoveDButton()->show();
//      panel->getFavoriteButton()->set_sensitive(true);
//      panel->getTrashButton()->set_sensitive(true);
    }

 
/*
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
*/
  }
}

Glib::ustring TTUDLRHider::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(  getExpander()->getEnabled() ? "1" : "0") ;
  Glib::ustring s_hide_arrows = getToolName() + ":"  + "hide_arrows " + std::string( cbHideArrow->get_active() ? "1" : "0" ) ;
  Glib::ustring s_lock_fav = getToolName() + ":"  + "lock_fav " + std::string( cbLockFav->get_active() ? "1" : "0" ) ;
  return s_active + "\n" +  s_hide_arrows + "\n" + s_lock_fav + "\n";
}

void TTUDLRHider::themeImport(std::ifstream& myfile)
{
  std::string line;
  bool condition = true;
  while (condition)
  {
    int position = myfile.tellg();
    condition = static_cast<bool> (getline (myfile,line));

    std::istringstream tokensplitter(line);
    std::string token;
//    printf("parsing line: %s\n", line.c_str());
    if (getline(tokensplitter, token, ':'))
    {
//      printf("Checking %s \n", token.c_str());
      if (token == getToolName())
      {
        if(getline(tokensplitter, token, ' '))
        {
          if (token == "active")
          {
            if(getline(tokensplitter, token, ' '))
            { 
              getExpander()->setEnabled((token == "1") ? true: false);
              printf("conf loading: enabled=%s\n", token.c_str());

              printf("enabled=%i hidarrow=%i lockFav=%i \n", (getExpander()->getEnabled()?1:0), (cbHideArrow->get_active()?1:0), (cbLockFav->get_active()?1:0));

            }
          }

          if (token == "hide_arrows")
          {
            if(getline(tokensplitter, token, ' '))
            {
              cbHideArrow->set_active((token == "1") ? true: false);
              printf("conf loading: settings hide arrow=%s\n", token.c_str()); 
              printf("enabled=%i hidarrow=%i lockFav=%i \n", (getExpander()->getEnabled()?1:0), (cbHideArrow->get_active()?1:0), (cbLockFav->get_active()?1:0));
            }
          }

         if (token == "lock_fav")
         {
            if(getline(tokensplitter, token, ' '))
            {
              cbLockFav->set_active((token == "1") ? true: false);
              printf("conf loading: settings lock_fav=%s\n", token.c_str()); 
              printf("enabled=%i hidarrow=%i lockFav=%i \n", (getExpander()->getEnabled()?1:0), (cbHideArrow->get_active()?1:0), (cbLockFav->get_active()?1:0));
            }
         }
       }
     }
     else
     {
        //we restore the position since it is a line for another tool that we read.
        myfile.seekg(position);
        condition = false;
     }
    }
  }
}

