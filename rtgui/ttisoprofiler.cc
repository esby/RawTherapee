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
#include "ttisoprofiler.h"
#include "options.h"
#include "guiutils.h"
#include "rtimage.h"
#include <sstream>
#include <fstream>

#include "ttdep.h"

using namespace rtengine;
using namespace rtengine::procparams;


TTIsoProfiler::TTIsoProfiler () : FoldableToolPanel(this,"TTIsoProfiler",M("TT_ISO_PROFILER"),false,true)
{
	themeBox1 = Gtk::manage(new Gtk::HBox());
	themeBox1->set_spacing(4);

	themeProfilePickerLabel = Gtk::manage(new Gtk::Label(M("TTP_PROFILE")));
        themeProfilePickerLabel->set_width_chars(6);
        themeIsoLabel = Gtk::manage(new Gtk::Label(M("TTP_ISO")));
        themeIsoLabel->set_width_chars(6);



        buttonSave = Gtk::manage(new Gtk::Button());
        buttonSave->set_image (*Gtk::manage(new RTImage ("spiral")));

	entry = Gtk::manage (new Gtk::Entry ());
        entry->set_width_chars(6);

        themeBox1->pack_start(*themeIsoLabel,Gtk::PACK_SHRINK, 0);

        themeBox1->pack_start(*entry, Gtk::PACK_SHRINK, 0);

	themeBox1->pack_start(*themeProfilePickerLabel, Gtk::PACK_SHRINK, 0);

        profilbox = Gtk::manage (new MyComboBoxText ());
        profilbox->set_tooltip_text (M("TP_WAVELET_TILES_TOOLTIP"));
        themeBox1->pack_start(*profilbox); //, Gtk::PACK_SHRINK, 0);
        themeBox1->pack_start(*buttonSave, Gtk::PACK_SHRINK, 0);
     
	pack_start( *themeBox1, Gtk::PACK_SHRINK, 0);

        themeBox2 = Gtk::manage(new Gtk::HBox());
        themeBox2->set_spacing(4);

       pack_start( *themeBox2);

       themeBox3 = Gtk::manage(new Gtk::VBox());
       themeBox3->set_spacing(4);
       

       pack_start( *themeBox3, Gtk::PACK_SHRINK, 0);

       parseProfileFolder();

       buttonSave->signal_button_release_event().connect_notify( sigc::mem_fun(*this, &TTIsoProfiler::save_clicked) );
       profilbox->signal_changed ().connect (sigc::mem_fun (*this, &TTIsoProfiler::profileBoxChanged));
}

void TTIsoProfiler::deploy()
{
/*
  if (options.TTPAutoload)
  {
    auto it = std::find(entries.begin(), entries.end(), options.TTPAutoloadValue ) ;
    if (it != entries.end())
    {
      int index= std::distance(entries.begin(), it);
//      printf("autoloading index=%i \n" ,index);
      profilbox->set_active(index);
    }
  }
*/
}

void TTIsoProfiler::react(FakeProcEvent ev)
{
   if ((ev == FakeEvExifTransmitted )
   || (ev == FakeEvProfileChanged))
   {
     printf("TTIsoProfiler::react() \n");
     if  (getExpander()->getEnabled())
     {
       printf("expander enabled \n");
       RtVariable *v = env->getVariableByName("Iso");
       if (v == nullptr) {
         printf("iso rtvariable not available yet \n");
         return;
       }

       Glib::ustring iso_as_string = v->toString();

       int iso = atoi(env->getVariableByName("Iso")->toString().c_str());

       size_t i;
       for (i=0; i<listIsos.size(); i++)
       {
         int isolimit = atoi(listIsos[i].c_str());
//         printf("%s p Checking iso= %i vs isolimit= %i #%zu \n", getToolName().c_str(),  iso, isolimit, i);
         if (isolimit > iso) 
         {
//           printf("%s excessing profile found, previous one will be used\n",getToolName().c_str());
           break;
         }
       }
       
       // we load the i-1 profile, supposed it exists.
       i = i -1;
       printf("DEBUG: %s i=%zu listIsos.size()=%zu \n", getToolName().c_str(), i, listIsos.size());
       if ((i>=0) && (i<listIsos.size()))
       {
//         printf("loading profile= %s \n", listPaths[i].c_str());
         printf("%s: loading profile: %s \n",getToolName().c_str(),listPaths[i].c_str());
         load_profile(listPaths[i]);
       }
       }
    }
}


void TTIsoProfiler::profileBoxChanged()
{
   Glib::ustring name =  profilbox->get_active_text() ;
   int row = profilbox->get_active_row_number();
   Glib::ustring path = paths[row];
   printf("%s preparing to load Filename [%s] - %s",getToolName().c_str(), name.c_str(), + path.c_str());
   load_profile(path);
   if( options.rtSettings.verbose ) 
     printf("Loaded profile : %s - %s\n", name.c_str(), path.c_str());
}

void TTIsoProfiler::parseProfileFolder()
{
   Glib::ustring lp1 = options.getUserProfilePath();
   Glib::ustring lp2 = options.getGlobalProfilePath();

   Glib::ustring p1 = Glib::build_filename(lp1,"Partial");
   Glib::ustring p2 = Glib::build_filename(lp2,"Partial");

//   printf("p1= %s \n",p1.c_str());
//   printf("p2= %s \n",p2.c_str());
    
   Glib::ustring realPath;
   Glib::ustring currDir;
   Glib::ustring virtualPath;
   Glib::ustring folder;

   int nbpass = 0;
   do 
   {
        // the code is a simplified version of ProfileStore::ParseDir 
        // from profilestore.cc

        // walking through the directory
        Glib::Dir* dir = nullptr;
        if (nbpass == 0) 
          realPath = p1;
        else 
          realPath = p2;

        if (file_test(realPath, Glib::FILE_TEST_IS_DIR))
        {
        dir = new Glib::Dir (realPath);

        for (Glib::DirIterator i = dir->begin(); i != dir->end(); ++i) {
            currDir = *i;

            if (currDir == "." || currDir == "..") {
                continue;
            }

            Glib::ustring fname = Glib::build_filename(realPath, currDir);
            //todo: this used to be safe_file_test, check this does not cause issue under windows
            if (file_test (fname, Glib::FILE_TEST_IS_DIR)) {
            } else {
                size_t lastdot = currDir.find_last_of ('.');

                if (lastdot != Glib::ustring::npos && lastdot <= currDir.size() - 4 && !currDir.casefold().compare (lastdot, 4, paramFileExtension)) {
                    // file found
                    if( options.rtSettings.verbose ) {
                        printf ("ttp profile detected %s... \n", fname.c_str());
                    }

                    Glib::ustring name = currDir.substr(0, lastdot);
    
                    int ps1 = fname.find(lp1);
                    if(ps1>-1)
                    {
                      fname = "${U}" + fname.substr(lp1.length(), fname.length() - lp1.length() );
                      fname = fname.substr(0,fname.find(name)) + name;
                    }

                    int ps2 = fname.find(lp2);
                    if(ps2>-1)
                    {
                      fname = "${G}" + fname.substr(lp2.length(), fname.length() - lp2.length() );
                      fname = fname.substr(0,fname.find(name)) + name;
                    }

//                    printf("fname= %s \n",fname.c_str());
//                    printf("name=%s\n",name.c_str());
                    profilbox->append(name);
                    paths.push_back(fname);
                    names.push_back(name);
                }
            }
        }
       delete dir;
       }
       nbpass++;
  }
  while  (nbpass<2);
}

void TTIsoProfiler::load_profile(Glib::ustring path)
{
  printf("%s changing profile to %s \n", getToolName().c_str(), path.c_str());
  env->getProfilePanel()->changeProfile(path);

}

void TTIsoProfiler::save_profile(Glib::ustring iso, Glib::ustring name, Glib::ustring path)
{
   size_t i;
   for (i=0; i<listIsos.size(); i++)
     if (listIsos[i] == iso) break;
   
   if (i == listIsos.size() )
   {
     listNames.push_back(name);
     listIsos.push_back(iso);
     listPaths.push_back(path);
   }
   else
   {
     listNames[i] = name;
     listIsos[i] = iso;
     listPaths[i] = path;
   }

}

void TTIsoProfiler::affect_profiles()
{
  size_t i;
  sortVectorsByIsos();

//  printf("%s, listing partial iso profiles \n",getToolName().c_str());
  for (i=0; i<listIsos.size(); i++)
  {
    Glib::ustring name = listNames[i];
    Glib::ustring iso  = listIsos[i];
//    printf("%s, %zu isolimit= %s name= %s \n",getToolName().c_str(),i, name.c_str(), iso.c_str());
  }


  Gtk::Entry* entryName;
  Gtk::Entry* entryIso;

 // size_t i;
  for (i=0; i<listIsos.size(); i++)
  {
     if (i<entryIsos.size())
     {
       entryName = entryNames[i];
       entryIso = entryIsos[i];

       entryName->set_text(listNames[i]);
       entryIso->set_text(listIsos[i]);
       entryName->show();
       entryIso->show();
       entryDelButtons[i]->show();
     }
     else
     {
       entryName = Gtk::manage (new Gtk::Entry ());
       entryIso = Gtk::manage (new Gtk::Entry ());

       entryName->set_width_chars(15);
       entryIso->set_width_chars(6);

       entryName->set_text(listNames[i]);
       entryIso->set_text(listIsos[i]);



       Gtk::Button *buttonDel = Gtk::manage(new Gtk::Button());
       buttonDel->set_image (*Gtk::manage(new RTImage ("trash")));

       entryNames.push_back(entryName);
       entryIsos.push_back(entryIso);
       entryDelButtons.push_back(buttonDel);


       Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());

       hbox->pack_start(*entryIso, Gtk::PACK_SHRINK,0);
       hbox->pack_start(*entryName, Gtk::PACK_SHRINK, 0);   
       hbox->pack_start(*buttonDel, Gtk::PACK_SHRINK, 0);   
//       buttonDel->signal_button_release_event().connect_notify( sigc::mem_fun(*this, &TTIsoProfiler::delete_clicked, i) );
       buttonDel->signal_clicked().connect( sigc::bind<int>( sigc::mem_fun(*this, &TTIsoProfiler::delete_clicked),i ));
//      signal_enabled_toggled().connect ( sigc::bind( sigc::mem_fun(this, &Wavelet::enableToggled), expresid) );



       themeBox3->pack_start(*hbox);
       entryName->show();
       entryIso->show();
       hbox->show();

     }
  }

  for (i=listIsos.size(); i<entryIsos.size(); i++)
  {
    entryNames[i]->hide();
    entryIsos[i]->hide();
    entryDelButtons[i]->hide();
  }


}

void TTIsoProfiler::delete_clicked (int pos)
{
  if ((pos > -1) && ((size_t)pos<listPaths.size()))
  {
   listPaths.erase(listPaths.begin() + pos);
   listNames.erase(listNames.begin() + pos);
   listIsos.erase(listIsos.begin() + pos);

   affect_profiles();

  }
}


void TTIsoProfiler::autoload_clicked (GdkEventButton* event)
{
  /*
  if (event->button != 1) {
        return;
    }

  Glib::ustring filename =  profilbox->get_active_text() + paramFileGuiExtension;
  Glib::ustring fname = "";

  int row = profilbox->get_active_row_number();
  if (row >-1) // if a profile was selectd
  {
    fname = entries[row];
  }

  //the call happends before the value is changed, so we use a not
  options.TTPAutoload = not cbAutoloadSettings->get_active();

  if (!options.TTPAutoload)
  {
    fname = "";
  }

  options.TTPAutoloadValue =  fname;
  lbAutoloadSettingsLine->set_text(fname);
  options.save();
*/
}

bool isNumber(const Glib::ustring &line) 
{
 if (line[0] == '0') return true;
 return (atoi(line.c_str()));
}

void TTIsoProfiler::save_clicked (GdkEventButton* event)
{
   Glib::ustring name;
   auto idname = profilbox->get_active_row_number();
   if (idname>-1)
     name = profilbox->get_active_text();
   else return;

   auto it = std::find(names.begin(), names.end(), name ) ;
   int index= std::distance(names.begin(), it);

   
   Glib::ustring path = paths[index];

   Glib::ustring isolimit =  entry->get_text();
   if (!isNumber(isolimit)) 
     return;
   
   printf("recording iso= %s name= %s path=%s \n", isolimit.c_str(), name.c_str(), path.c_str());
   save_profile(isolimit, name, path);
   affect_profiles();
 
}


Glib::ustring TTIsoProfiler::themeExport()
{

  Glib::ustring settings = "";
  settings = getToolName() + ":" + "active:" + std::string(  getExpander()->getEnabled() ? "1" : "0") + "\n" ;

  size_t i;
  for (i=0; i < entryIsos.size(); i++)
     settings = settings +  getToolName() + ":"  + listIsos[i] + ":" +  listNames[i] + ":" + listPaths[i] + "\n";

  printf("Saving ttp format: \n %s \n", settings.c_str());
  return settings;  

}

void TTIsoProfiler::themeImport(std::ifstream& myfile)
{

  std::string line;
  bool condition = true;
  while (condition)
  {
    int position = myfile.tellg();
    condition = static_cast<bool> (getline (myfile,line));

    std::istringstream tokensplitter(line);
    std::string token;

    std::string iso;
    
    std::string name;
    std::string path;
 
    if (options.rtSettings.verbose)
      printf("parsing line: %s\n", line.c_str());

    if (getline(tokensplitter, token, ':'))
    {
      if (token == getToolName())
      {
        if (getline(tokensplitter, token, ':'))
        {

         if (token == "active")
          {
            if(getline(tokensplitter, token, ':'))
            {
              getExpander()->setEnabled((token == "1") ? true: false);
            }
          }
          else
          {

           iso = token;
           if (getline(tokensplitter, token, ':'))
           {
             name = token;
             if (getline(tokensplitter, token,'\n'))
             {
               path = token;
               if (options.rtSettings.verbose)
                 printf("ttp: %s read values: iso= %s name= %s path=%s \n", getToolName().c_str(),  iso.c_str(), name.c_str(), path.c_str());
               save_profile(iso,name,path);
             }
           }
           }              
        }
      }
      else
      {
        //we restore the position since it is a line for another tool that we read.
        myfile.seekg(position);
        condition = false;

        affect_profiles();
      }
    }
  }
}


void TTIsoProfiler::sortVectorsByIsos() 
{

  size_t i;
  size_t j;

  for (i=0; i<listIsos.size(); i++)
    for (j=i+1; j<listIsos.size();j++)
     {
         int v1 = atoi(listIsos[i].c_str());
         int v2 = atoi(listIsos[j].c_str());
         if (v1 > v2)
         {

           Glib::ustring tmp1 = listIsos[i];
           listIsos[i] = listIsos[j];
           listIsos[j] = tmp1;

           Glib::ustring tmp2 = listNames[i];
           listNames[i] = listNames[j];
           listNames[j] = tmp2;

           Glib::ustring tmp3 = listPaths[i];
           listPaths[i] = listPaths[j];
           listPaths[j] = tmp3;
           
         }
   
     }
}
