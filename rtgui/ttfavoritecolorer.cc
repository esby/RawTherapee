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
#include "ttfavoritecolorer.h"
#include "options.h"
#include "guiutils.h"
//#include "../rtengine/safegtk.h"
#include "rtimage.h"
#include <fstream>

using namespace rtengine;
using namespace rtengine::procparams;

TTFavoriteColorChooser::TTFavoriteColorChooser () : FoldableToolPanel(this,"ttfavoritecolorer",M("TP_FAVORITE_COLORER_LABEL"),false,true)
{
  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);
  themeBox2 = Gtk::manage(new Gtk::HBox());
  themeBox2->set_spacing(4);

  lbFavorite = Gtk::manage(new Gtk::Label(M("TT_LABELPICKER_FAVORITE")));
  cbFavorite = Gtk::manage(new Gtk::ColorButton());

  lbTrash = Gtk::manage(new Gtk::Label(M("TT_LABELPICKER_TRASH")));
  cbTrash = Gtk::manage(new Gtk::ColorButton());

  themeBox1->pack_start(*lbFavorite, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_end(*cbFavorite, Gtk::PACK_SHRINK, 0);

  themeBox2->pack_start(*lbTrash, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_end(*cbTrash, Gtk::PACK_SHRINK, 0);


  pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox2, Gtk::PACK_SHRINK, 0);

  Gdk::Color color;
  color.set("blue");
  cbFavorite->set_color(color);
  color.set("red");
  cbTrash->set_color(color);

}

void TTFavoriteColorChooser::deploy()
{
   FoldableToolPanel::deploy();
   for (size_t i=0; i< env->countPanel() ; i++)
   { 
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {
 //       printf("connecting to %s \n", p->getToolName().c_str());
        p->getFavoriteButton()->signal_clicked().connect( sigc::bind<ToolPanel*>( sigc::mem_fun(this, &TTFavoriteColorChooser::colorFavorite), p, false  ));
        p->getTrashButton()->signal_clicked().connect( sigc::bind<ToolPanel*>( sigc::mem_fun(this, &TTFavoriteColorChooser::colorTrash), p, false  ));
        colorFavorite(p, true);
        colorTrash(p, true);
      }
   }

  // button enable / disable
  getExpander()->signal_enabled_toggled().connect( sigc::mem_fun(this, &TTFavoriteColorChooser::enabledChanged));
}

void TTFavoriteColorChooser::read(const rtengine::procparams::ProcParams* pp, const ParamsEdited* pedited)
{

}

void TTFavoriteColorChooser::write( rtengine::procparams::ProcParams* pp, ParamsEdited* pedited)
{

}

//void TTFavoriteColorChooser::on_toggle_button()
void TTFavoriteColorChooser::enabledChanged()
{  
   //  if (getEnabledButton()->get_active()) 
   if (getExpander()->getEnabled())
   for (size_t i=0; i< env->countPanel() ; i++)
   {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {
        colorFavorite(p);
        colorTrash(p);
      }
  }
  else
  for (size_t i=0; i< env->countPanel() ; i++)
  {
     FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {
        colorFavorite(p, true);
        colorTrash(p, true);
      }
  }
}

void  TTFavoriteColorChooser::colorButton(Gtk::ToggleButton* button, Gdk::Color color, bool deactivate)
{  
  bool state;
  Gtk::Widget* w;

  w  = (Gtk::Widget*) button;
  state = button->get_active();
  
 // if((!deactivate) && (getEnabledButton()->get_active()))
  if((!deactivate) && (getExpander()->getEnabled()))
  {


    if ((!deactivate) && (state))
    {
      w->modify_bg(Gtk::STATE_NORMAL , color);
      w->modify_bg(Gtk::STATE_ACTIVE , color);
      w->modify_bg(Gtk::STATE_PRELIGHT, color);
    }
    else
    {
      w->unset_bg(Gtk::STATE_NORMAL);
      w->unset_bg(Gtk::STATE_ACTIVE);
      w->unset_bg(Gtk::STATE_PRELIGHT);
    }
  }
  else
  {
      w->unset_bg(Gtk::STATE_NORMAL);
      w->unset_bg(Gtk::STATE_ACTIVE);
      w->unset_bg(Gtk::STATE_PRELIGHT);
  }
   
}

void TTFavoriteColorChooser::colorFavorite(ToolPanel* panel, bool deactivate)
{
  if (panel!=NULL)
  {
    Gtk::ToggleButton* b = panel->getFavoriteButton();
    colorButton(b, cbFavorite->get_color(), deactivate);
  }
}

void TTFavoriteColorChooser::colorTrash(ToolPanel* panel, bool deactivate)
{
  if (panel!=NULL)
  {
    Gtk::ToggleButton* b = panel->getTrashButton();
    colorButton(b, cbTrash->get_color(), deactivate);
  }
}

Glib::ustring TTFavoriteColorChooser::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(  getExpander()->getEnabled() ? "1" : "0") ;
  Glib::ustring s_fav_color = getToolName() + ":"  + "favorite_color " +  cbFavorite->get_color().to_string();
  Glib::ustring s_trash_color = getToolName() + ":"  + "trash_color " +  cbTrash->get_color().to_string();
  return s_active + "\n" +  s_fav_color + "\n" + s_trash_color + "\n";
}


void TTFavoriteColorChooser::themeImport(std::ifstream& myfile)
{
  std::string line;
  bool condition = true;
  while (condition)
  {
    int position = myfile.tellg();
    condition = static_cast<bool> getline (myfile,line);

    std::istringstream tokensplitter(line);
    std::string token;
//    printf("parsing line: %s\n", line.c_str());
    if (getline(tokensplitter, token, ':'))
    {
//       printf("Checking %s \n", token.c_str());
      if (token == getToolName())
      {
        if(getline(tokensplitter, token, ' '))
        {
          if (token == "active")
          {
            if(getline(tokensplitter, token, ' '))
            { 
              getExpander()->setEnabled((token == "1") ? true: false);
            }
          }

          if (token == "favorite_color")
          {
            if(getline(tokensplitter, token, ' '))
            {
              Gdk::Color color;
              //todo: verify that this method is safe against buffer overlow
              color.set(token);
              cbFavorite->set_color(color);
            }
          }

         if (token == "trash_color")
         {
            if(getline(tokensplitter, token, ' '))
            {
              Gdk::Color color;
              //todo: verify that this method is safe against buffer overlow
              color.set(token); 
              cbTrash->set_color(color);
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


