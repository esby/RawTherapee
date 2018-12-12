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
#include "tttweaker.h"
#include "options.h"
#include "guiutils.h"
#include "rtimage.h"
#include "distortion.h"
#include "whitebalance.h"
#include <chrono>
#include <thread>
#include <fstream>

using namespace rtengine;
using namespace rtengine::procparams;

extern void exit(int exit_code);


TTTweaker::TTTweaker() : FoldableToolPanel(this,"TTTweaker",M("TT_TWEAKER_LABEL"),false,false)
{
  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);

  lbAutoDistortionCorrect = Gtk::manage(new Gtk::Label(M("TT_TWEAKER_AUTO_DISTORTION_CORRECTION")));
  cbAutoDistortionCorrect = Gtk::manage(new Gtk::CheckButton());

  themeBox1->pack_start(*lbAutoDistortionCorrect, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_end(*cbAutoDistortionCorrect, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);

  themeBox2 = Gtk::manage(new Gtk::HBox());
  themeBox2->set_spacing(4);

  lbCloseAfterSave = Gtk::manage(new Gtk::Label(M("TT_TWEAKER_CLOSE_AFTER_SAVE_IN_SE")));
  cbCloseAfterSave = Gtk::manage(new Gtk::CheckButton());

  themeBox2->pack_start(*lbCloseAfterSave, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_end(*cbCloseAfterSave, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox2, Gtk::PACK_SHRINK, 0);

  themeBox3 = Gtk::manage(new Gtk::HBox());
  themeBox3->set_spacing(4);

  lbReduceAfterSave = Gtk::manage(new Gtk::Label(M("TT_TWEAKER_REDUCE_AFTER_SAVE_IN_ME")));
  cbReduceAfterSave = Gtk::manage(new Gtk::CheckButton());

  pack_start(*themeBox3, Gtk::PACK_SHRINK, 0);

  themeBox3->pack_start(*lbReduceAfterSave, Gtk::PACK_SHRINK, 0);
  themeBox3->pack_end(*cbReduceAfterSave, Gtk::PACK_SHRINK, 0);

  themeBox4 = Gtk::manage(new Gtk::HBox());
  themeBox4->set_spacing(4);

  lbResetWBForRt4Profiles = Gtk::manage(new Gtk::Label(M("TT_TWEAKER_RESET_WB_FOR_RT4_PP_FILES")));
  cbResetWBForRt4Profiles = Gtk::manage(new Gtk::CheckButton());

  themeBox4->pack_start(*lbResetWBForRt4Profiles, Gtk::PACK_SHRINK, 0);
  themeBox4->pack_end(*cbResetWBForRt4Profiles, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox4, Gtk::PACK_SHRINK, 0);
 
}

void TTTweaker::deploy()
{
  FoldableToolPanel::deploy();
  env->registerPriority(getToolName());

  // button enable / disable

//   getExpander()->signal_enabled_toggled().connect(sigc::mem_fun(this, &TTTweaker::enabledChanged));
//   cbAutoDistortionCorrect->signal_clicked().connect( sigc::mem_fun(this, &TTTweaker::enabledChanged));

}

void TTTweaker::react(FakeProcEvent ev)
{
  int benchmark = env->getVarAsInt("benchmark");
//  printf("benchmark=%i\n",benchmark);
  if ((ev == FakeEvPP3Transmitted)
  && benchmark == 1)
  {
    printf("exiting the application for benchmark purpose\n");
    exit(0); // because gtk_main_quit() will not work here...
  }
   

  
  if ((cbAutoDistortionCorrect->get_active())
  && (env->checkPriority(getToolName())))
  {
    if ((ev == FakeEvPhotoLoaded)
    || (ev == FakeEvProfileChanged))
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
          env->setPriority(getToolName());
          d->idPressed();
        }
      }
    }
  }
  if ((ev == FakeEvFileSaved)) 
  {
    if (cbCloseAfterSave->get_active() && simpleEditor)
    {
      // sleep(1); // note: sleep might not be multi platform
      std::this_thread::sleep_for(std::chrono::seconds(1));
      printf("exiting the program after file saving was performed\n");
      gtk_main_quit();
    }

    if (cbReduceAfterSave->get_active() && !simpleEditor)
    {
      // sleep(1); // note: sleep might not be multi platform
      std::this_thread::sleep_for(std::chrono::seconds(1));
      printf("Reducting the program after file saving was performed\n");
      getToplevelWindow(this).iconify();
    }
  }

 if (cbResetWBForRt4Profiles->get_active())
  {
    if (ev == FakeEvPP3Transmitted)
    for (size_t i=0; i< env->countPanel() ; i++)
    {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != nullptr)
      && (!(p->canBeIgnored())))
      {
        if (p->getToolName() == "whitebalance")
        {
          WhiteBalance* w = static_cast<WhiteBalance*> (p);
          int pp3version = env->getVarAsInt("pp3version");
          printf("reading pp3version=%i \n", pp3version);
          if ((pp3version > -1) && (pp3version < 329))
          {
            printf("Resetting whitebalance to camera.\n");
            w->resetWBToCamera();
          }
        }
      }
    }
  }

}

void TTTweaker::enabledChanged  () 
{  
}

Glib::ustring TTTweaker::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(  getExpander()->getEnabled() ? "1" : "0") ;
  Glib::ustring s_enable_auto = getToolName() + ":"  + "enable_auto_distortion_correction " + std::string( cbAutoDistortionCorrect->get_active() ? "1" : "0" ) ;
  Glib::ustring s_close_after_save = getToolName() + ":"  + "enable_close_after_save " + std::string( cbCloseAfterSave->get_active() ? "1" : "0" ) ;
  Glib::ustring s_reduce_after_save = getToolName() + ":"  + "enable_reduce_after_save " + std::string( cbReduceAfterSave->get_active() ? "1" : "0" ) ;
  Glib::ustring s_reset_wb = getToolName() + ":"  + "enable_reset_wb " + std::string( cbResetWBForRt4Profiles->get_active() ? "1" : "0" ) ;


  return s_active + "\n" \
       + s_enable_auto + "\n" \
       + s_close_after_save + "\n" \
       + s_reduce_after_save + "\n" \
       + s_reset_wb + "\n" \
;
}

void TTTweaker::deployLate()
{
}

void TTTweaker::themeImport(std::ifstream& myfile)
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

          if (token == "enable_close_after_save")
          {
            if(getline(tokensplitter, token, ' '))
            {
              cbCloseAfterSave->set_active((token == "1") ? true: false);
            }
          }

          if (token == "enable_auto_distortion_correction")
          { 
            if(getline(tokensplitter, token, ' '))
            { 
              cbAutoDistortionCorrect->set_active((token == "1") ? true: false);
            }
          }

          if (token == "enable_reduce_after_save")
          {
            if(getline(tokensplitter, token, ' '))
            {
              cbReduceAfterSave->set_active((token == "1") ? true: false);
            }
          }

          if (token == "enable_reset_wb")
          {
            if(getline(tokensplitter, token, ' '))
            {
              cbResetWBForRt4Profiles->set_active((token == "1") ? true: false);
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

