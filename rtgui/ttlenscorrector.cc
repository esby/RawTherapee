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
#include "ttlenscorrector.h"
#include "options.h"
#include "guiutils.h"
#include "rtimage.h"
#include "variable.h"
#include <chrono>
#include <thread>
#include <fstream>

using namespace rtengine;
using namespace rtengine::procparams;

extern void exit(int exit_code);


TTLensCorrector::TTLensCorrector() : FoldableToolPanel(this,"TTLensCorrector",M("TTLENS_CORRECTOR_LABEL"),false,false)
{
  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);

  lbDistortionCorrect = Gtk::manage(new Gtk::Label(M("TTLENS_CORRECTOR_DIST")));
  cbDistortionCorrect = Gtk::manage(new Gtk::CheckButton());

  themeBox1->pack_start(*lbDistortionCorrect, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_end(*cbDistortionCorrect, Gtk::PACK_SHRINK, 0);

  pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);
}

void TTLensCorrector::deploy()
{
  FoldableToolPanel::deploy();

  for (size_t i=0; i< env->getToolPanels().size() ; i++)
  {
    ToolPanel* p = static_cast<ToolPanel*> (env->getPanel(i));
    if ( (p != NULL))
    {
       if (p->getToolName() == "distortion")        
           distortion = static_cast<Distortion*> (p);       
    }
  }
}

void TTLensCorrector::react(FakeProcEvent ev)
{

  if (cbDistortionCorrect->get_active())
  {
   if (ev == FakeEvFileSaved)
   {

     if (value_not_found)
     {
       std::ofstream lensfile(lenspath, std::ios::app);
       if (lensfile) 
       { // Vérifie si l'ouverture a réussi
         double dv = distortion->getDistorValue();
         RtVariable* d = env->getVariableByName("FLen");
         int fl = std::stoi(d->toString());
         printf("%f %i \n", dv, fl);
         lensfile << fl << " " << dv << std::endl; // Écrit et ajoute un saut de ligne
       } 
       else 
       {
         std::cerr << "Erreur d'ouverture du fichier !" << std::endl;
       }   
     }
   }

   if (ev == FakeEvPhotoLoaded )//FakeEvExifTransmitted )
//   || (ev == FakeEvProfileChanged))
      if (distortion != nullptr)
      {
          printf("applying distorsion correction.\n");
          env->setVar("disableAutoDistortionCorrection",true);
  load_data();

  int key;

  RtVariable* d = env->getVariableByName("FLen");

  key = std::stoi(d->toString());
  if (vid.find(key) != vid.end()) 
  {
    double d = vid[key];
    printf("found %i with %f \n", key, d);
    value_not_found=false;
    distortion->setDistorValue(d);
  } 
  else 
  {
    printf("no value found for %i \n", key);
    value_not_found=true;
  }

      }
  }

}

void TTLensCorrector::enabledChanged  () 
{  
}

Glib::ustring TTLensCorrector::themeExport()
{
  Glib::ustring s_active = getToolName() + ":" + "active " + std::string(  getExpander()->getEnabled() ? "1" : "0") ;
  Glib::ustring s_enable_distorsion_correction = getToolName() + ":"  + "enable_distortion_correction " + std::string( cbDistortionCorrect->get_active() ? "1" : "0" ) ;

  return s_active + "\n" \
       + s_enable_distorsion_correction +"\n" ;
}

void TTLensCorrector::deployLate()
{
}

void TTLensCorrector::themeImport(std::ifstream& myfile)
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

          if (token == "enable_distortion_correction")
          { 
            if(getline(tokensplitter, token, ' '))
            { 
              cbDistortionCorrect->set_active((token == "1") ? true: false);
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

void TTLensCorrector::load_data()
{
    const char* home = std::getenv("HOME"); // Récupère le chemin du home
    if (!home) {
        std::cerr << "Impossible de récupérer la variable $HOME" << std::endl;
        //todo proper exit
        return;
    }
    std::string lensname = env->getVarAsString("Lens");
    if (lensname.empty())
      lensname = env->getVarAsString("Camera");
    if (lensname.empty())
      lensname = "Camera";

    std::replace(lensname.begin(), lensname.end(), '/', ' ');
    std::replace(lensname.begin(), lensname.end(), '\\', ' ');
    // todo a
    #ifdef _WIN32
    const char SEPARATOR = '\\';  // Windows
    #else
    const char SEPARATOR = '/';   // Linux/macOS
    #endif
    lenspath = std::string(home) + SEPARATOR + lensname + ".txt"; 


    std::ifstream fichier(lenspath);
    if (!fichier) {
        std::cerr << "Erreur lors de l'ouverture du fichier : " << lenspath << std::endl;
      return;
        //todo proper exit
    }

    int valeur_entière;
    double valeur_décimale;

    while (fichier >> valeur_entière >> valeur_décimale) {
        vid[valeur_entière] = valeur_décimale; // Stockage dans la map
    }

    fichier.close();
}
