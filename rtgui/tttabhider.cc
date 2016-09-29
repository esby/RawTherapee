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
#include "tttabhider.h"
#include "options.h"
#include "guiutils.h"
#include "rtimage.h"
#include <sstream>
#include <fstream>
#include "toolpanelcoord.h"


using namespace rtengine;
using namespace rtengine::procparams;


TTTabHider::TTTabHider () : FoldableToolPanel(this,"tttabhider",M("TP_THEMETOOL__OPTIONS_LABEL"),false,true)
{
  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);

  themeBox2 = Gtk::manage(new Gtk::HBox());
  themeBox2->set_spacing(4);

  lbHideFavorite = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_FAVORITE")));
  cbHideFavorite = Gtk::manage(new Gtk::CheckButton());

  lbHideTrash = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_TRASH")));
  cbHideTrash = Gtk::manage(new Gtk::CheckButton());

  themeBox1->pack_start(*lbHideFavorite, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_end(*cbHideFavorite, Gtk::PACK_SHRINK, 0);

  themeBox2->pack_start(*lbHideTrash, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_end(*cbHideTrash, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox2, Gtk::PACK_SHRINK, 0);
}

void TTTabHider::deploy()
{
  getExpander()->signal_enabled_toggled().connect(sigc::mem_fun(this, &TTTabHider::enabledChanged));
  cbHideFavorite->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_favorite_clicked));
  cbHideTrash->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_trash_clicked));
}

void TTTabHider::enabled_changed()
{

   //todo: find a way to disable the checkbox

}

void TTTabHider::hide_favorite_clicked()
{ 
  env->getToolPanelNotebook()->get_nth_page(0)->set_visible(not cbHideFavorite->get_active() );
}

void TTTabHider::hide_trash_clicked()
{
// basically the metadata panel is removed from the batchcoordinator process
// this means the number of panels displayed is to be added by 1 when it is not removed 
// for getting the trash panel position; which is the last position.
// maybe another way to check it could be implemented
  int nb_panel = NB_PANEL;
  if (env->getMetadataState())
    nb_panel++;
/* // debugging code to understand what was happening 
  for (int i=0; i< nb_panel; i++){
  Gtk::ScrolledWindow* box = dynamic_cast<Gtk::ScrolledWindow*> (env->getToolPanelNotebook()->get_nth_page(i));

 //   env->getToolPanelNotebook()->get_nth_page(i)->set_visible(true);
  
 // this is for accessing container, but we have a bin which is not a container
 //  std::vector<Gtk::Widget*> panelList = box->get_children ();
  if (box)
  {

     Gtk::Viewport* vp = (Gtk::Viewport*) box->get_child();
     ToolVBox* v = (ToolVBox*) vp->get_child();
     printf("%s\n", v->getBoxName().c_str());
  }  
  else printf("box is not a bin");
 //  printf("%s\n",env->getToolPanelNotebook()->get_nth_page(i)->get_name().c_str());

 }
*/
  env->getToolPanelNotebook()->get_nth_page(nb_panel-1)->set_visible(not cbHideTrash->get_active() );

}


Glib::ustring TTTabHider::themeExport()
{
  Glib::ustring favSettings = getToolName() + ":"  + "favorite:";
  Glib::ustring traSettings = getToolName() + ":"  + "trash:";

  favSettings += "visible " + env->getToolPanelNotebook()->get_nth_page(0)->get_visible() ?  "1": "0"; 
  traSettings += "visible " + env->getToolPanelNotebook()->get_nth_page(NB_PANEL +  -1)->get_visible() ?  "1": "0";

  return favSettings + "\n" +  traSettings + "\n"; 
}



void TTTabHider::themeImport(std::ifstream& myfile)
{

  std::string line;
  bool condition = true;
  while (condition)
  {
    int position = myfile.tellg();
    condition = getline (myfile,line);

    std::istringstream tokensplitter(line);
    std::string token;
    printf("parsing line: %s\n", line.c_str());
    if (getline(tokensplitter, token, ':'))
    {
      if (token == getToolName())
      {
        if (getline(tokensplitter, token, ':'))
        {
          if (token == "favorite")
          {
            while(getline(tokensplitter, token, ' '))
            {
              env->getToolPanelNotebook()->get_nth_page(0)->set_visible(token == "1"); 
            }
          }

          if (token == "trash")
          { 
            while(getline(tokensplitter, token, ' '))
            {
              env->getToolPanelNotebook()->get_nth_page(NB_PANEL-1)->set_visible(token== "1");
            }
          }
        }
      }else
      {  
         //we restore the position since it is a line for another tool that we read.
         myfile.seekg(position);
         condition = false;
      }
    }
  }
}
