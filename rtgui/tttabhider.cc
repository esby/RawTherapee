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


TTTabHider::TTTabHider () : FoldableToolPanel(this,"tttabhider",M("TP_TT_TAB_HIDDER"),false,false)
{
  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);

  themeBox2 = Gtk::manage(new Gtk::HBox());
  themeBox2->set_spacing(4);
  themeBox3 = Gtk::manage(new Gtk::HBox());
  themeBox3->set_spacing(4);
  themeBox4 = Gtk::manage(new Gtk::HBox());
  themeBox4->set_spacing(4);
  themeBox5 = Gtk::manage(new Gtk::HBox());
  themeBox5->set_spacing(4);
  themeBox6 = Gtk::manage(new Gtk::HBox());
  themeBox6->set_spacing(4);
  themeBox7 = Gtk::manage(new Gtk::HBox());
  themeBox7->set_spacing(4);
  themeBox8 = Gtk::manage(new Gtk::HBox());
  themeBox8->set_spacing(4);
  themeBox9 = Gtk::manage(new Gtk::HBox());
  themeBox9->set_spacing(4);
  themeBox10 = Gtk::manage(new Gtk::HBox());
  themeBox10->set_spacing(4);
  themeBox11 = Gtk::manage(new Gtk::HBox());
  themeBox11->set_spacing(4);


  buttonSave = Gtk::manage(new Gtk::Button());
  buttonSave->set_image (*Gtk::manage(new RTImage ("save.png")));

  lbHideFavorite = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_FAVORITE")));
  cbHideFavorite = Gtk::manage(new Gtk::CheckButton());

  lbHideExposure = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_EXPOSURE")));
  cbHideExposure = Gtk::manage(new Gtk::CheckButton());

  lbHideDetails = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_DETAIL")));
  cbHideDetails = Gtk::manage(new Gtk::CheckButton());

  lbHideColor = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_COLOR")));
  cbHideColor = Gtk::manage(new Gtk::CheckButton());

  lbHideAdvanced = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_ADVANCED")));
  cbHideAdvanced = Gtk::manage(new Gtk::CheckButton());

  lbHideLocal = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_LOCAL")));
  cbHideLocal = Gtk::manage(new Gtk::CheckButton());

  lbHideTransform = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_TRANSFORM")));
  cbHideTransform = Gtk::manage(new Gtk::CheckButton());

  lbHideRaw = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_RAW")));
  cbHideRaw = Gtk::manage(new Gtk::CheckButton());

  lbHideMetadata = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_METADATA")));
  cbHideMetadata = Gtk::manage(new Gtk::CheckButton());

  lbHideUseful = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_USEFUL")));
  cbHideUseful = Gtk::manage(new Gtk::CheckButton());

  lbHideTrash = Gtk::manage(new Gtk::Label(M("TP_TT_HIDE_TRASH")));
  cbHideTrash = Gtk::manage(new Gtk::CheckButton());

  themeBox1->pack_start(*cbHideFavorite, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_start(*lbHideFavorite, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_start(*cbHideExposure, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_start(*lbHideExposure, Gtk::PACK_SHRINK, 0);
  themeBox3->pack_start(*cbHideDetails, Gtk::PACK_SHRINK, 0);
  themeBox3->pack_start(*lbHideDetails, Gtk::PACK_SHRINK, 0);
  themeBox4->pack_start(*cbHideColor, Gtk::PACK_SHRINK, 0);
  themeBox4->pack_start(*lbHideColor, Gtk::PACK_SHRINK, 0);
  themeBox5->pack_start(*cbHideAdvanced, Gtk::PACK_SHRINK, 0);
  themeBox5->pack_start(*lbHideAdvanced, Gtk::PACK_SHRINK, 0);
  themeBox6->pack_start(*cbHideLocal, Gtk::PACK_SHRINK, 0);
  themeBox6->pack_start(*lbHideLocal, Gtk::PACK_SHRINK, 0);
  themeBox7->pack_start(*cbHideTransform, Gtk::PACK_SHRINK, 0);
  themeBox7->pack_start(*lbHideTransform, Gtk::PACK_SHRINK, 0);
  themeBox8->pack_start(*cbHideRaw, Gtk::PACK_SHRINK, 0);
  themeBox8->pack_start(*lbHideRaw, Gtk::PACK_SHRINK, 0);
  themeBox9->pack_start(*cbHideMetadata, Gtk::PACK_SHRINK, 0); 
  themeBox9->pack_start(*lbHideMetadata, Gtk::PACK_SHRINK, 0);
  themeBox10->pack_start(*lbHideUseful, Gtk::PACK_SHRINK, 0);
  themeBox10->pack_end(*cbHideUseful, Gtk::PACK_SHRINK, 0);  // we invert the order because it will make the tool invisible...
  themeBox11->pack_start(*cbHideTrash, Gtk::PACK_SHRINK, 0);
  themeBox11->pack_start(*lbHideTrash, Gtk::PACK_SHRINK, 0);


  pack_start(*buttonSave, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox2, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox3, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox4, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox5, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox6, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox7, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox8, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox9, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox10, Gtk::PACK_SHRINK, 0);

  reacted = false;

}

void TTTabHider::deploy()
{
  getExpander()->signal_enabled_toggled().connect(sigc::mem_fun(this, &TTTabHider::enabledChanged));

  buttonSave->signal_button_release_event().connect_notify( sigc::mem_fun(*this, &TTTabHider::save_clicked) );
}

void TTTabHider::deployLate()
{
  cbHideFavorite->set_active (options.TTPHideFavorite);
  cbHideExposure->set_active (options.TTPHideExposure);
  cbHideDetails->set_active (options.TTPHideDetails);
  cbHideColor->set_active (options.TTPHideColor);
  cbHideAdvanced->set_active (options.TTPHideAdvanced);
  cbHideLocal->set_active (options.TTPHideLocal);
  cbHideTransform->set_active (options.TTPHideTransform);
  cbHideRaw->set_active (options.TTPHideRaw);
  cbHideMetadata->set_active (options.TTPHideMetadata);
  cbHideUseful->set_active (options.TTPHideUseful);
  cbHideTrash->set_active (options.TTPHideTrash);

  cbHideFavorite->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_favorite_clicked));
  cbHideExposure->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_exposure_clicked));
  cbHideDetails->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_details_clicked));
  cbHideColor->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_color_clicked));
  cbHideAdvanced->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_advanced_clicked));
  cbHideLocal->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_local_clicked));
  cbHideTransform->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_transform_clicked));
  cbHideRaw->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_raw_clicked));
  cbHideMetadata->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_metadata_clicked));
  cbHideUseful->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_useful_clicked));
  cbHideTrash->signal_clicked().connect( sigc::mem_fun(this, &TTTabHider::hide_trash_clicked));

}


void TTTabHider::react(FakeProcEvent ev)
{
        if (ev == FakeEvExifTransmitted)
        {  
          if (reacted == false)
          {             
            hide_favorite_clicked ();
            hide_exposure_clicked ();
            hide_details_clicked ();
            hide_color_clicked ();
            hide_advanced_clicked ();
            hide_local_clicked ();
            hide_transform_clicked ();
            hide_raw_clicked ();
            hide_metadata_clicked ();
            hide_useful_clicked ();
            hide_trash_clicked ();
            reacted=true;
          }
        }

}

void TTTabHider::enabled_changed()
{

   //todo: find a way to disable the checkbox

}

void TTTabHider::save_clicked(GdkEventButton* event)
{
if (event->button != 1) {
        return;
    }
  options.save();

}

void TTTabHider::hide_favorite_clicked()
{ 
  env->getToolPanelNotebook()->get_nth_page(0)->set_visible(not cbHideFavorite->get_active() );
  options.TTPHideFavorite = cbHideFavorite->get_active();
}

void TTTabHider::hide_exposure_clicked ()
{ 
  env->getToolPanelNotebook()->get_nth_page(1)->set_visible(not cbHideExposure->get_active() );
  options.TTPHideExposure = cbHideExposure->get_active();
}

void TTTabHider::hide_details_clicked ()
{ 
  env->getToolPanelNotebook()->get_nth_page(2)->set_visible(not cbHideDetails->get_active() );
  options.TTPHideDetails = cbHideDetails->get_active();
}

void TTTabHider::hide_color_clicked ()
{ 
  env->getToolPanelNotebook()->get_nth_page(3)->set_visible(not cbHideColor->get_active() );
  options.TTPHideColor = cbHideColor->get_active();
}

void TTTabHider::hide_advanced_clicked ()
{ 
  env->getToolPanelNotebook()->get_nth_page(4)->set_visible(not cbHideAdvanced->get_active() );
  options.TTPHideAdvanced = cbHideAdvanced->get_active();
}

void TTTabHider::hide_local_clicked ()
{
  printf("hiding local clicked, cbHideLocal=%d options.TTPHideLocal=%d \n",   
    cbHideLocal->get_active(), options.TTPHideLocal );
  env->getToolPanelNotebook()->get_nth_page(5)->set_visible(not cbHideLocal->get_active() );
  options.TTPHideLocal = cbHideLocal->get_active();
}

void TTTabHider::hide_transform_clicked ()
{ 
  env->getToolPanelNotebook()->get_nth_page(6)->set_visible(not cbHideTransform->get_active() );
  options.TTPHideTransform = cbHideTransform->get_active();
}

void TTTabHider::hide_raw_clicked ()
{ 
  env->getToolPanelNotebook()->get_nth_page(7)->set_visible(not cbHideRaw->get_active() );
  options.TTPHideRaw = cbHideRaw->get_active();
}

void TTTabHider::hide_metadata_clicked ()
{ 
  if (env->getMetadataState()) 
  {
    env->getToolPanelNotebook()->get_nth_page(8)->set_visible(not cbHideMetadata->get_active() );
  }
  options.TTPHideMetadata= cbHideMetadata->get_active();
}

void TTTabHider::hide_useful_clicked ()
{ 
  int increment = 0;
  if (env->getMetadataState()) 
     increment = 1;
  env->getToolPanelNotebook()->get_nth_page(8+increment)->set_visible(not cbHideUseful->get_active() );
  options.TTPHideUseful = cbHideUseful->get_active();
}


void TTTabHider::hide_trash_clicked()
{
  int increment = 0;
  if (env->getMetadataState()) 
     increment = 1;
  env->getToolPanelNotebook()->get_nth_page(9+increment)->set_visible(not cbHideTrash->get_active() );
  options.TTPHideTrash = cbHideTrash->get_active();

/* old code
// basically the metadata panel is removed from the batchcoordinator process
// this means the number of panels displayed is to be added by 1 when it is not removed 
// for getting the trash panel position; which is the last position.
// maybe another way to check it could be implemented
  int nb_panel = NB_PANEL;
  if (env->getMetadataState())
    nb_panel++;
*/
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
// env->getToolPanelNotebook()->get_nth_page(nb_panel-1)->set_visible(not cbHideTrash->get_active() );

}


Glib::ustring TTTabHider::themeExport()
{
//todo: this needs to be recoded to take all panels into account
//TODO : missing other panels definitions
  Glib::ustring favSettings = getToolName() + ":"  + "favorite:";
  Glib::ustring traSettings = getToolName() + ":"  + "trash:";

/*
  Glib::ustring exposureSettings = getToolName() + ":"  + "exposure:";
  Glib::ustring detailsSettings = getToolName() + ":"  + "details:";
  Glib::ustring colorSettings = getToolName() + ":"  + "color:";
  Glib::ustring advancedSettings = getToolName() + ":"  + "advanced:";
  Glib::ustring transformSettings = getToolName() + ":"  + "transform:";
  Glib::ustring rawSettings = getToolName() + ":"  + "raw:";
  Glib::ustring metadataSettings = getToolName() + ":"  + "metadata:";
  Glib::ustring usefulSettings = getToolName() + ":"  + "useful:";
*/
  favSettings       += "visible " + env->getToolPanelNotebook()->get_nth_page(0)->get_visible() ?  "1": "0"; 
/*  exposureSettings  += "visible " + cbHideExposure->get_active()  ?  "1": "0";  
  detailsSettings   += "visible " + cbHideDetails->get_active()  ?  "1": "0";
  colorSettings     += "visible " + cbHideColor->get_active()  ?  "1": "0";
  advancedSettings  += "visible " + cbHideAdvanced->get_active()  ?  "1": "0";
  transformSettings += "visible " + cbHideTransform->get_active()  ?  "1": "0";
  rawSettings       += "visible " + cbHideRaw->get_active()  ?  "1": "0";
  metadataSettings  += "visible " + cbHideMetadata->get_active()  ?  "1": "0";
  usefulSettings    += "visible " + cbHideUseful->get_active()  ?  "1": "0";
*/
  traSettings       += "visible " + cbHideTrash->get_active()  ?  "1": "0";

  return favSettings + "\n" 
/*      +  exposureSettings + "\n"
      +  detailsSettings + "\n"
      +  colorSettings + "\n"
      +  advancedSettings + "\n"
      +  transformSettings + "\n"
      +  rawSettings + "\n"
      +  metadataSettings + "\n"
      +  usefulSettings + "\n"*/
      +  traSettings + "\n"; 
}



void TTTabHider::themeImport(std::ifstream& myfile)
{

  std::string line;
  bool condition = true;
  while (condition)
  {
    int position = myfile.tellg();
    condition = static_cast<bool> (getline (myfile,line));

    std::istringstream tokensplitter(line);
    std::string token;
 //   printf("parsing line: %s\n", line.c_str());
    if (getline(tokensplitter, token, ':'))
    {
      if (token == getToolName())
      {
        if (getline(tokensplitter, token, ':'))
        {
          //todo this needs a recode to take all panels into account
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
