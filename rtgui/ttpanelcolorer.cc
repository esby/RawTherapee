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
#include "ttpanelcolorer.h"
#include "options.h"
#include "guiutils.h"
#include "rtimage.h"
#include <fstream>

using namespace rtengine;
using namespace rtengine::procparams;


TTPanelColorChooser::TTPanelColorChooser () : FoldableToolPanel(this, "ttpanelcolorer",M("TP_PANEL_COLORER_LABEL"),false,true)
{
  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);

  themeBox2 = Gtk::manage(new Gtk::HBox());
  themeBox2->set_spacing(4);

  lbPanel1 = Gtk::manage(new Gtk::Label(M("TT_LABELPICKER_PANEL_ALWAYS")));
  cbPanel1 = Gtk::manage(new Gtk::ColorButton());
  lbPanel2 = Gtk::manage(new Gtk::Label(M("TT_LABELPICKER_PANEL_CLICKED")));
  cbPanel2 = Gtk::manage(new Gtk::ColorButton());

  themeBox1->pack_start (*lbPanel1, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_end   (*cbPanel1, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_start (*lbPanel2, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_end   (*cbPanel2, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);
  pack_start(*themeBox2, Gtk::PACK_SHRINK, 0);

  Gdk::RGBA rgba;
  rgba.set("green");
  cbPanel1->set_rgba(rgba);
  rgba.set("orange");
  cbPanel2->set_rgba(rgba);

}

void TTPanelColorChooser::test()
{ 
  if (e->get_visible())
    e->hide();
  else
    e->show();

  printf("test\n");
}

void TTPanelColorChooser::deploy()
{
   FoldableToolPanel::deploy();
   for (size_t i=0; i< env->getToolPanels().size() ; i++)
   { 
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != nullptr)
      && (!(p->canBeIgnored())))
      {
//        printf("connecting to %s \n", p->getToolName().c_str());
//        printf("panel: %s exp=%p\n",p->getToolName().c_str(),p->getExpander());
        p->getExpander()->signal_enabled_toggled().connect( sigc::bind(sigc::mem_fun(*this, &TTPanelColorChooser::colorer) , p ));
        if (p->getExpander()->getEnabled())
        {
          // we react on click
          p->getExpander()->signal_enabled_toggled().connect( sigc::bind<ToolPanel*>( sigc::mem_fun(this, &TTPanelColorChooser::colorer) , p )); 
        }

        colorPanel(p, true);
        pangoPanel(p, true);

         // we also react on update on the info box. (this means the label in the top part of the expander
        p->getLabelInfoNotifier()->signal_activate()  .connect( sigc::bind<ToolPanel*>( sigc::mem_fun(this, &TTPanelColorChooser::pangorer), p ));

      }
   }
   // button enable / disable
   getExpander()->signal_enabled_toggled().connect( sigc::mem_fun(this, &TTPanelColorChooser::on_toggle_button));
}

void TTPanelColorChooser::on_toggle_button() 
{
   if (getExpander()->getEnabled())
   // if (getEnabledButton()->get_active())
   for (size_t i=0; i< env->getToolPanels().size() ; i++)
   {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != nullptr)
      && (!(p->canBeIgnored())))
      {
        colorPanel(p);
        pangoPanel(p);
      }
  }
  else
  for (size_t i=0; i< env->getToolPanels().size() ; i++)
  {
     FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != nullptr)
      && (!(p->canBeIgnored())))
      {
        colorPanel(p, true); 
        pangoPanel(p, true);
      }
  }

}

int TTPanelColorChooser::getState(ToolPanel* panel)
{
  int state;
  state = -1;
  // checking for this tt to be active
  if (getExpander()->getEnabled())
  {
    state = 0;
    if (!panel->getExpander()->getUseEnabled())
      state = 1;
    else
    if (panel->getExpander()->getEnabled())
      state = 2;
    }
  return state;  

/*
//getEnabledButton()->get_active())
  {
    state = 0;
    if (panel->getEnabledButton() == nullptr)
      state = 2;
    else
    if (panel->getEnabledButton()->get_active())
      state = 1;
  }
  return state;
*/
}

void TTPanelColorChooser::react(FakeProcEvent ev)
{
  if (ev == FakeEvShowAllTriggered)
  {
    on_toggle_button();
  }
}


Gdk::RGBA TTPanelColorChooser::getColor(ToolPanel* panel, int state)
{
  Gdk::RGBA rgba;
   rgba = cbPanel1->get_rgba();
   if (state>1)
     rgba = cbPanel2->get_rgba();
  return rgba;
}

void TTPanelColorChooser::pangoPanel(ToolPanel* panel, bool deactivate)
{
  Gtk::Label* l0;
 // Gtk::Label* l1;
  Gtk::HBox* b = (Gtk::HBox*) panel->getLabelBox();
  std::vector<Gtk::Widget*> v = b->get_children();
  //l1 = (Gtk::Label*) v.at(2);


  //todo fix issue and disable return
  if (!panel->getNeed100Percent())
  {
  l0 = dynamic_cast<Gtk::Label*> (v.at(1));
  if (l0)
  { 
    Glib::ustring s0 = l0->get_text();
    Glib::ustring s1 = panel->getFavoriteButton()->get_label();
 
    s0 = escapeHtmlChars(s0);
//    s1 = escapeHtmlChars(s1);
    int state;
    state = getState(panel);
    //  printf("state %i - %s - %s \n", state,  s0.c_str(), s1.c_str());
    if ((deactivate) || (state>-1))
    { 
       if (!deactivate &&  (state>0))
      { 
        Gdk::RGBA rgba;
        rgba = getColor(panel, state);
        Glib::ustring colorName = getHtmlColor(rgba);
        l0->set_markup("<tt><span color=\"" + colorName + "\">" +  s0 + "</span></tt>");

      }
      else
      {
//       printf("s0=%s\n",s0.c_str());
        l0->set_markup("<tt><span>" + s0 + "</span></tt>");

      }
    }
  }
  }
}

void TTPanelColorChooser::colorPanel(ToolPanel* panel, bool deactivate) 
{
  Gdk::RGBA rgba;  
  int state;
//  Gtk::Widget* w;

//todo: fix colorer whic does not work anymore
   
//  w = (Gtk::Widget*) panel->getExpander();

//  Gtk::Widget* w2;
//  w2->hide();
 // Gtk::Container* c = (Gtk::Container*) panel->getExpander();
//  std::vector<Gtk::Widget*> v = c->get_children();
//  w  = (Gtk::Widget*) v.at(0);

  state = getState(panel);

  if (deactivate) // called when clicking on deactivate
    state = 0;
  
  if (state>-1)
  {
    if (state>0)
    { 
      rgba = getColor(panel, state);
//      panel->getTrashButton()->set_sensitive(false);
    }
    else
    {

    }
  }
   
}

void TTPanelColorChooser::colorer(ToolPanel* panel)
{
  if (panel!=nullptr)
  {
    colorPanel(panel);
    pangoPanel(panel);
  }
}



void TTPanelColorChooser::pangorer(ToolPanel* panel)
{
  if (panel!=nullptr)
  {
    pangoPanel(panel);
  }
}

Glib::ustring TTPanelColorChooser::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(  getExpander()->getEnabled() ? "1" : "0") ;
  Glib::ustring s_fav_color = getToolName() + ":"  + "always_color " +  cbPanel1->get_rgba().to_string();
  Glib::ustring s_trash_color = getToolName() + ":"  + "clicked_color " +  cbPanel2->get_rgba().to_string();
  return s_active + "\n" +  s_fav_color + "\n" + s_trash_color + "\n";
}

void TTPanelColorChooser::themeImport(std::ifstream& myfile)
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

          if (token == "always_color")
          {
            if(getline(tokensplitter, token, ' '))
            {
              Gdk::RGBA rgba;
              //todo: verify that this method is safe against buffer overlow
              rgba.set(token);
              cbPanel1->set_rgba(rgba);
            }
          }

         if (token == "clicked_color")
         {
            if(getline(tokensplitter, token, ' '))
            {
              Gdk::RGBA rgba;
              //todo: verify that this method is safe against buffer overlow
              rgba.set(token); 
              cbPanel2->set_rgba(rgba);
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




