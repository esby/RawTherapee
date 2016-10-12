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
#include "ttautodistortionclicker.h"
#include "options.h"
#include "guiutils.h"
#include "rtimage.h"
#include "distortion.h"
#include <fstream>

using namespace rtengine;
using namespace rtengine::procparams;

TTAutoDistortionClicker::TTAutoDistortionClicker() : FoldableToolPanel(this,"ttautodistortionclicker",M("TT_AUTO_DISTORTION_CLICKER_LABEL"),false,false)
{
  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);

  lbAutoDistortionCorrect = Gtk::manage(new Gtk::Label(M("TT_AUTO_DISTORTION_CORRECTION")));
  cbAutoDistortionCorrect = Gtk::manage(new Gtk::CheckButton());

  themeBox1->pack_start(*lbAutoDistortionCorrect, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_end(*cbAutoDistortionCorrect, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);

}

void TTAutoDistortionClicker::deploy()
{
  FoldableToolPanel::deploy();
  // button enable / disable

//   getExpander()->signal_enabled_toggled().connect(sigc::mem_fun(this, &TTAutoDistortionClicker::enabledChanged));
//   cbAutoDistortionCorrect->signal_clicked().connect( sigc::mem_fun(this, &TTAutoDistortionClicker::enabledChanged));

}

void TTAutoDistortionClicker::react()
{
  if (cbAutoDistortionCorrect->get_active())
   for (size_t i=0; i< env->countPanel() ; i++)
   {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != NULL)
      && (!(p->canBeIgnored())))
      {
        if (p->getToolName() == "distortion")
        {
          printf("Clicking on auto distorsion correction button.\n");
          Distortion* d = static_cast<Distortion*> (p);
          d->idPressed();
        }
      }
  }
}

void TTAutoDistortionClicker::enabledChanged  () 
{  
}

Glib::ustring TTAutoDistortionClicker::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(  getExpander()->getEnabled() ? "1" : "0") ;
  Glib::ustring s_enable_auto = getToolName() + ":"  + "enable_auto_Distortion_correction " + std::string( cbAutoDistortionCorrect->get_active() ? "1" : "0" ) ;
  return s_active + "\n" +  s_enable_auto + "\n";
}

void TTAutoDistortionClicker::deployLate()
{
}

void TTAutoDistortionClicker::themeImport(std::ifstream& myfile)
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
            }
          }

          if (token == "enable_auto_Distortion_correction")
          {
            if(getline(tokensplitter, token, ' '))
            {
              cbAutoDistortionCorrect->set_active((token == "1") ? true: false);
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

