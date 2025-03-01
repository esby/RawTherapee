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
#include "variable.h"
#include <chrono>
#include <thread>
#include <fstream>
#include <exiv2/exiv2.hpp>

using namespace rtengine;
using namespace rtengine::procparams;

extern void exit(int exit_code);


TTTweaker::TTTweaker() : FoldableToolPanel(this,"TTTweaker",M("TTTW_LABEL"),false,false)
{
  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);

  lbAutoDistortionCorrect = Gtk::manage(new Gtk::Label(M("TTTW_AUTO_DISTOR_CORR")));
  cbAutoDistortionCorrect = Gtk::manage(new Gtk::CheckButton());

  themeBox1->pack_start(*lbAutoDistortionCorrect, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_end(*cbAutoDistortionCorrect, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);

  themeBox2 = Gtk::manage(new Gtk::HBox());
  themeBox2->set_spacing(4);

  lbCloseAfterSave = Gtk::manage(new Gtk::Label(M("TTTW_CLOSE_AFTER_SAVE_SE")));
  cbCloseAfterSave = Gtk::manage(new Gtk::CheckButton());

  themeBox2->pack_start(*lbCloseAfterSave, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_end(*cbCloseAfterSave, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox2, Gtk::PACK_SHRINK, 0);

  themeBox3 = Gtk::manage(new Gtk::HBox());
  themeBox3->set_spacing(4);

  lbReduceAfterSave = Gtk::manage(new Gtk::Label(M("TTTW_MIN_AFTER_SAVE_ME")));
  cbReduceAfterSave = Gtk::manage(new Gtk::CheckButton());

  pack_start(*themeBox3, Gtk::PACK_SHRINK, 0);

  themeBox3->pack_start(*lbReduceAfterSave, Gtk::PACK_SHRINK, 0);
  themeBox3->pack_end(*cbReduceAfterSave, Gtk::PACK_SHRINK, 0);

  themeBox4 = Gtk::manage(new Gtk::HBox());
  themeBox4->set_spacing(4);

  lbResetWBForRt4Profiles = Gtk::manage(new Gtk::Label(M("TTTW_RESET_WB_RT4_PPT")));
  cbResetWBForRt4Profiles = Gtk::manage(new Gtk::CheckButton());

  themeBox4->pack_start(*lbResetWBForRt4Profiles, Gtk::PACK_SHRINK, 0);
  themeBox4->pack_end(*cbResetWBForRt4Profiles, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox4, Gtk::PACK_SHRINK, 0);

  themeBox5 = Gtk::manage(new Gtk::HBox());

  lbAutoRotateCorrect = Gtk::manage(new Gtk::Label(M("TTTW_AUTO_HORI_CORR")));
  cbAutoRotateCorrect = Gtk::manage(new Gtk::CheckButton());

  themeBox5->pack_start(*lbAutoRotateCorrect, Gtk::PACK_SHRINK, 0);
  themeBox5->pack_end(*cbAutoRotateCorrect, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox5, Gtk::PACK_SHRINK, 0);

  themeBox6 = Gtk::manage(new Gtk::HBox());

  lbToolNameAsToolTip = Gtk::manage(new Gtk::Label(M("TTTW_TOOLNAME_AS_TOOLTIP")));
  cbToolNameAsToolTip = Gtk::manage(new Gtk::CheckButton());

  themeBox6->pack_start(*lbToolNameAsToolTip, Gtk::PACK_SHRINK, 0);
  themeBox6->pack_end(*cbToolNameAsToolTip, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox6, Gtk::PACK_SHRINK, 0);

  themeBox7 = Gtk::manage(new Gtk::HBox());

  lbToolNameUntranslated = Gtk::manage(new Gtk::Label(M("TTTW_TOOLNAME_UNTRANSLATED")));
  cbToolNameUntranslated = Gtk::manage(new Gtk::CheckButton());

  themeBox7->pack_start(*lbToolNameUntranslated, Gtk::PACK_SHRINK, 0);
  themeBox7->pack_end(*cbToolNameUntranslated, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox7, Gtk::PACK_SHRINK, 0);

}

void TTTweaker::deploy()
{
  FoldableToolPanel::deploy();

  env->setVar("disableAutoDistortionCorrection",false);

  for (size_t i=0; i< env->getToolPanels().size() ; i++)
  {
    ToolPanel* p = static_cast<ToolPanel*> (env->getPanel(i));
    if ( (p != NULL))
    {
       if (p->getToolName() == "rotate")
         rotate = static_cast<Rotate*> (p);
       if (p->getToolName() == "whitebalance")
         whitebalance = static_cast<WhiteBalance*> (p);
       if (p->getToolName() == "coarse")
         coarse = static_cast<CoarsePanel*> (p);
       if (p->getToolName() == "distortion")
         distortion = static_cast<Distortion*> (p);
    }
  }


  // button enable / disable

//   getExpander()->signal_enabled_toggled().connect(sigc::mem_fun(this, &TTTweaker::enabledChanged));
//   cbAutoDistortionCorrect->signal_clicked().connect( sigc::mem_fun(this, &TTTweaker::enabledChanged));
   cbToolNameAsToolTip->signal_clicked().connect( sigc::mem_fun(this, &TTTweaker::enabledChanged));
   cbToolNameUntranslated->signal_clicked().connect( sigc::mem_fun(this, &TTTweaker::enabledChanged));
}

float normalizeRotation(float f)
{
  int sign;
  if ( f>0 )
    sign = +1;
  else
    sign = -1;
            
  while (f*sign>45)
    f = f +180*sign*-1;

  return f;
}

float checkCoarse(float f, CoarsePanel* coarse)
{
  while (f>45) 
   {
    printf("coarse rotating to the left\n");
     f = f -90;
     coarse->rotateLeft();
   }

  while (f<-45)
  {
    printf("coarse rotating to the right\n");
    f = f +90;
    coarse->rotateRight();
  }
  return f;
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
  
  if (cbAutoDistortionCorrect->get_active())
  {
   if ((ev == FakeEvExifTransmitted )
   || (ev == FakeEvProfileChanged))
      if (distortion != nullptr)
      {
          bool d = env->getVarAsBool("disableAutoDistortionCorrection");
          if ( !d )
          {
            printf("Clicking on auto distorsion correction button.\n");
            distortion->idPressed();
          }
      }
  }

  if (cbAutoRotateCorrect->get_active())
   if ((ev == FakeEvExifTransmitted )
   || (ev == FakeEvProfileChanged))
    if ((rotate != nullptr)
    && (coarse != nullptr))
    {
      check_exif(); 
      Glib::ustring s_burst = env->getExifVariable("Exif:MakerNote:BurstMode");
      printf("DEBUG: Exif:MakerNote:BurstMode= %s \n", s_burst.c_str());
      if (s_burst == "0")
      {
        Glib::ustring s_roll= env->getExifVariable("Exif:MakerNote:RollAngle");
        printf("Rotating angle d=%s \n",s_roll.c_str());
        int i = atoi(s_roll.c_str()) ;
        short d;
        d = (short)i;
        d = d / 10;

        printf("Rotating angle d=%i \n",d);
        if (d!=0)
        {
          Glib::ustring s_orientation = env->getExifVariable("Exif:MakerNote:CameraOrientation");
          if (s_orientation == "Horizontal (normal)")
          {
            d = -d;          
            d = checkCoarse(d, coarse);
          }
          else if (s_orientation == "Rotate CW") ///"Rotate 90 CW")
          {
            d = 90-d;
            d = checkCoarse(d, coarse);
          }
          else if (s_orientation == "Rotate 180")
          {
            d = -d;
            d = normalizeRotation(d);
            //correcting a bug
            coarse->rotateRight();
            coarse->rotateRight();
          }
          else if (s_orientation == "Rotate CCW") // the normal orientation is  "Rotate 270 CW") 
          {
            d = -90-d;
            d = checkCoarse(d, coarse);
            printf("rotate CCW detected d adjusted to d=%i \n",d);
          }
          //other cases not handled
          rtengine::procparams::ProcParams* pp;
          pp = new ProcParams();
          rotate->write(pp);
          if ((pp->rotate.degree == 0) 
          &&  (pp->rotate.degree != d))
          {
            pp->rotate.degree = d;
            rotate->read(pp);
            rotate->adjusterChanged(nullptr, d);
            printf("%s auto rotating by degree=%i \n",getToolName().c_str(),d);
          }
        }
      }
    }

  if (ev == FakeEvFileSaved)
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
   if ((ev == FakeEvExifTransmitted )
   || (ev == FakeEvProfileChanged))
      if (whitebalance != nullptr)
      {
        int pp3version = env->getVarAsInt("pp3version");
        printf("reading pp3version=%i \n", pp3version);
        if ((pp3version > -1) && (pp3version < 329))
        {
          printf("Resetting whitebalance to camera.\n");
          whitebalance->resetWBToCamera();
        }
      }
  }

}

void TTTweaker::enabledChanged  () 
{  
 for (size_t i=0; i< env->getToolPanels().size() ; i++)
  {
    ToolPanel* p = static_cast<ToolPanel*> (env->getPanel(i));
    if ( (p != NULL))
    {
//      auto udlrbox = p->getFUDLRBox();
      auto ptoolname = p->getToolName();

      MyExpander* e = p->getExpander();
      if (e)
      {
        Gtk::Widget* w = e->getLabelWidget();
        if (w)
        {
          Gtk::Label* l = static_cast<Gtk::Label*>(w);
          if (l)
          {
             if (cbToolNameUntranslated->get_active())
                l->set_label(ptoolname);
             else
                l->set_label(p->getOriginalUILabel());
              
            if (cbToolNameAsToolTip->get_active())
            {
              l->set_tooltip_markup(escapeHtmlChars(ptoolname));
            }
            else 
              l->set_tooltip_markup("");
          }
        }
      }
    }
  }
}

Glib::ustring TTTweaker::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(  getExpander()->getEnabled() ? "1" : "0") ;
  Glib::ustring s_enable_auto = getToolName() + ":"  + "enable_auto_distortion_correction " + std::string( cbAutoDistortionCorrect->get_active() ? "1" : "0" ) ;
  Glib::ustring s_close_after_save = getToolName() + ":"  + "enable_close_after_save " + std::string( cbCloseAfterSave->get_active() ? "1" : "0" ) ;
  Glib::ustring s_reduce_after_save = getToolName() + ":"  + "enable_reduce_after_save " + std::string( cbReduceAfterSave->get_active() ? "1" : "0" ) ;
  Glib::ustring s_reset_wb = getToolName() + ":"  + "enable_reset_wb " + std::string( cbResetWBForRt4Profiles->get_active() ? "1" : "0" ) ;
  Glib::ustring s_auto_rotate = getToolName() + ":"  + "enable_auto_rotate " + std::string( cbAutoRotateCorrect->get_active() ? "1" : "0" ) ;
  Glib::ustring s_toolname_as_tooltip = getToolName() + ":"  + "enable_toolname_as_tooltip " + std::string( cbToolNameAsToolTip->get_active() ? "1" : "0" ) ;
  Glib::ustring s_toolname_on_right = getToolName() + ":"  + "enable_toolname_untranslated " + std::string( cbToolNameUntranslated->get_active() ? "1" : "0" ) ;


  return s_active + "\n" \
       + s_enable_auto + "\n" \
       + s_close_after_save + "\n" \
       + s_reduce_after_save + "\n" \
       + s_reset_wb + "\n" \
       + s_auto_rotate + "\n" \
       + s_toolname_as_tooltip + "\n" \
       + s_toolname_on_right \
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

          if (token == "enable_auto_rotate")
          {
            if(getline(tokensplitter, token, ' '))
            {
              cbAutoRotateCorrect->set_active((token == "1") ? true: false);
            }
          }

          if (token == "enable_toolname_as_tooltip")
          {
            if(getline(tokensplitter, token, ' '))
            { 
              cbToolNameAsToolTip->set_active((token == "1") ? true: false);
            }
          }

          if (token == "enable_toolname_untranslated")
          {
            if(getline(tokensplitter, token, ' '))
            { 
              cbToolNameUntranslated->set_active((token == "1") ? true: false);
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

std::string getOrientationDescription(int orientation) {
    switch (orientation) {
        case 1: return "Horizontal (normal)";
        case 2: return "Mirrored horizontal";
        case 3: return "Upside-down (180°)";
        case 4: return "Mirrored vertical";
        case 5: return "Mirrored horizontal + 90° CW";
        case 6: return "Rotated 90° CW";
        case 7: return "Mirrored horizontal + 270° CW";
        case 8: return "Rotated 270° CW";
        default: return "Unknown orientation"; 
    }
}

void TTTweaker::check_exif()
{
 std::string cheminImage = env->getVarAsString("Fname");
 try {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(cheminImage);
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();

        if (exifData.empty()) {
            std::cerr << "Pas de données EXIF trouvées !" << std::endl;
            return ;
        }


// this code is used to find the exif tag to seek if needed
/*
 // tthis code
         for (const auto& exif : exifData) {
            std::cout << exif.key() << " = " << exif.value() << std::endl;
        }
*/

//todo
        // Recherche des tags Pitch et Roll

        auto burstmode = exifData.findKey(Exiv2::ExifKey("Exif.Panasonic.BurstMode"));
//       auto cameraorientation = exifData.findKey(Exiv2::ExifKey("Exif.Panasonic.CameraOrientation"));
        auto cameraorientation = exifData.findKey(Exiv2::ExifKey("Exif.Image.Orientation"));
        auto pitch = exifData.findKey(Exiv2::ExifKey("Exif.Panasonic.PitchAngle"));
        auto roll = exifData.findKey(Exiv2::ExifKey("Exif.Panasonic.RollAngle"));

        if (burstmode != exifData.end()) {
            Glib::ustring s = burstmode->value().toString();
            env->setVar(ROOT_EXIF_PREFIX + ":" + "Exif:MakerNote:BurstMode", s);

            std::cout << "burstmode : " << burstmode->value() << std::endl;
        } else {
            std::cout << "bustmode non trouvé !" << std::endl;
        } 

        if (cameraorientation != exifData.end()) {  
            int i = atoi(cameraorientation->value().toString().c_str());
            std::string s =  getOrientationDescription (i);

            std::cout << "cameraorientation r: " << cameraorientation->value() << std::endl;
            std::cout << "cameraorientation i: " << i << std::endl;
            std::cout << "cameraorientation s: " << s << std::endl;
            env->setVar(ROOT_EXIF_PREFIX + ":" + "Exif:MakerNote:CameraOrientation", s);
        } else {
            std::cout << "cameraorientation non trouvé !" << std::endl;
        }

        if (pitch != exifData.end()) {
            uint16_t u = exifData["Exif.Panasonic.PitchAngle"].toLong();  // Lecture en unsigned
            int16_t v = static_cast<int16_t>(u);  // Conversion en signé
            std::string s = std::to_string(v);
            env->setVar(ROOT_EXIF_PREFIX + ":" +"Exif:MakerNote:PicthAngle", s);
            std::cout << "Pitch Angle (s) : " << s << std::endl; // la valeur est multipliée par 10

        } else {
            std::cout << "Pitch Angle non trouvé !" << std::endl;
        }

        if (roll != exifData.end()) {
            uint16_t u = exifData["Exif.Panasonic.RollAngle"].toLong();  // Lecture en unsigned
            int16_t v = static_cast<int16_t>(u);  // Conversion en signé
            std::string s = std::to_string(v);
            env->setVar(ROOT_EXIF_PREFIX + ":" +"Exif:MakerNote:RollAngle", s);
            std::cout << "Roll Angle (s) : " << s << std::endl; // la valeur est multipliée par 10
        } else {
            std::cout << "Roll Angle non trouvé !" << std::endl;
        }
    } catch (Exiv2::Error &e) {
        std::cerr << "Erreur Exiv2 : " << e.what() << std::endl;
    }
}

