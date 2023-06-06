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
#include "ttsaver.h"
#include "options.h"
#include "guiutils.h"
#include "rtimage.h"
#include <sstream>
#include <fstream>

#include "ttdep.h"

using namespace rtengine;
using namespace rtengine::procparams;


TTSaver::TTSaver () : FoldableToolPanel(this,"ttsaver",M("TP_SAVER_LABEL"),false,false)
{
	themeBox1 = Gtk::manage(new Gtk::HBox());
	themeBox1->set_spacing(4);
	themeLabel = Gtk::manage(new Gtk::Label(M("GENERAL_FILE")));
        buttonSave = Gtk::manage(new Gtk::Button());
        buttonSave->set_image (*Gtk::manage(new RTImage ("save.png")));

	themeBox1->pack_start(*themeLabel, Gtk::PACK_SHRINK, 4);

        profilbox = Gtk::manage (new MyComboBoxText ());

        profilbox->set_tooltip_text (M("TP_SAVER_TOOLTIP"));
        themeBox1->pack_start(*profilbox, Gtk::PACK_EXPAND_WIDGET, 4); //, Gtk::PACK_SHRINK, 0);

//        auto v_cellRenderer = (Gtk::CellRendererText*)profilbox->get_first_cell();
//        v_cellRenderer->property_width_chars() = 1;
//        v_cellRenderer->property_ellipsize() = Pango::ELLIPSIZE_END;
//        v_cellRenderer->set_fixed_size(40, -1);;  

        themeBox1->pack_start(*buttonSave, Gtk::PACK_SHRINK, 4);
     
	pack_start( *themeBox1, Gtk::PACK_SHRINK, 4);

        themeBox2 = Gtk::manage(new Gtk::HBox());
        themeBox2->set_spacing(4);

        lbAutoloadSettings = Gtk::manage(new Gtk::Label(M("TT_SAVER_AUTOLOAD_SETTINGS")));
        cbAutoloadSettings = Gtk::manage(new Gtk::CheckButton());


       themeBox2->pack_start(*lbAutoloadSettings, Gtk::PACK_SHRINK, 4);
       themeBox2->pack_end(*cbAutoloadSettings, Gtk::PACK_SHRINK, 4);
       pack_start( *themeBox2, Gtk::PACK_SHRINK, 4);

       themeBox3 = Gtk::manage(new Gtk::HBox());
       themeBox2->set_spacing(4);
       
       enAutoloadSettingsLine = Gtk::manage(new Gtk::Entry ()); //options.TTPAutoloadValue));
       enAutoloadSettingsLine->set_width_chars(25);

       themeBox3->pack_start(*enAutoloadSettingsLine, Gtk::PACK_SHRINK, 4);
       pack_start( *themeBox3, Gtk::PACK_SHRINK, 4);

       parseProfileFolder();

       cbAutoloadSettings->set_active(options.TTPAutoload);
       buttonSave->signal_button_release_event().connect_notify( sigc::mem_fun(*this, &TTSaver::save_clicked) );
       cbAutoloadSettings->signal_button_release_event().connect_notify( sigc::mem_fun(*this, &TTSaver::autoload_clicked) );
       profilbox->signal_changed ().connect (sigc::mem_fun (*this, &TTSaver::profileBoxChanged));

}

TTSaver::~TTSaver()
{
    entries.clear();
}

void TTSaver::deploy()
{
   printf("DEBUG: TTSaver::deploy\n");
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
}

void TTSaver::profileBoxChanged()
{
   Glib::ustring filename =  profilbox->get_active_text() + paramFileGuiExtension;
   int row = profilbox->get_active_row_number();
   Glib::ustring fname = entries[row];
//   printf("preparing to load Filename : %s - %s\n", filename.c_str(), fname.c_str());
   load_ttp_profile(fname);
   if( options.rtSettings.verbose ) 
     printf("Loaded ttp profile : %s - %s\n", filename.c_str(), fname.c_str());
}

void TTSaver::parseProfileFolder()
{
   Glib::ustring p1 = options.getUserProfilePath();
   Glib::ustring p2 = options.getGlobalProfilePath();

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

                if (lastdot != Glib::ustring::npos && lastdot <= currDir.size() - 4 && !currDir.casefold().compare (lastdot, 4, paramFileGuiExtension)) {
                    // file found
                    if( options.rtSettings.verbose ) {
                        printf ("ttp profile detected %s... \n", fname.c_str());
                    }

                    Glib::ustring name = currDir.substr(0, lastdot);
                    printf("name=[%s]\n",name.c_str());
                    printf("fname=[%s]\n",fname.c_str());

                    profilbox->append(name);
                    entries.push_back(fname);

                }
            }
        }
       delete dir;
       nbpass++;
  }
  while  (nbpass<2);
}

//this function does two jobs:
// * reset the favorite / trash status of each panel.
// * call cleanBox to all panels, so they can be reaffected safely during load

void TTSaver::resetFavoriteAndTrashState() 
{
  std::vector<ToolPanel*> panels = env->getToolPanels();
  for (size_t i=0; i<panels.size(); i++)
  {
    if (!panels.at(i)->canBeIgnored())
    {
      panels.at(i)->getFavoriteButton()->set_active(false);
      panels.at(i)->getTrashButton()->set_active(false);
      ToolPanel* p = panels.at(i);
      p->cleanBox();
    }
  }
}

//this function job is to dispatch the lines between the current tools.

void TTSaver::themeSplitter(std::ifstream& myfile)
{
  env->disableSwitchPageReaction = true;
  printf("setting env_state to favorite");
  //todo this is wrong, it should be unchanged: either is it set to favorite at start, either to tool if executed from tool
  env->state = ENV_STATE_IN_FAV;
  env->prevState = ENV_STATE_IN_NORM;
  
  bool condition = true;
  std::string line = "";
  while (condition)
  {
    int position = myfile.tellg();
    condition = static_cast<bool> (getline (myfile,line));

    std::string currentToken = "";
    std::istringstream tokensplitter(line);
    std::string token;
//    printf("parsing line: %s\n", line.c_str());

    if (getline(tokensplitter, token, ':'))
    {
      if (currentToken != token)
      {
        if (currentToken == "")
        {
          currentToken = token;
        }
      }
      for (auto toolPanel : env->getToolPanels())
      {
        FoldableToolPanel* p = static_cast<FoldableToolPanel*> (toolPanel);
        if ( (p != nullptr)
        && (!(p->canBeIgnored())))
        {
          if (p->getToolName() == currentToken )
          { 
//            printf("Invoking themeImport on %s for %s\n",currentToken.c_str(),line.c_str());
            myfile.seekg(position);
            p->themeImport(myfile);
          }
        }
      }
   }

  
 }
 env->disableSwitchPageReaction = false;

//printf("panel positionning done\n");

// activating enabled buttons for toolpanel of usefulPanel
// because their configuration depends of reloading the button.
 for (size_t i=0; i< env->getToolPanels().size() ; i++)
      {
        FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));

/*        if ( (p != nullptr)
        && !(p->canBeIgnored()))
          printf("boxname=%s \n",p->getOriginalBox()->getBoxName().c_str());
*/
        if ( (p != nullptr)
        && (!(p->canBeIgnored()))
        && (p->getOriginalBox()->getBoxName() == "usefulPanel"))
        {        
           p->getExpander()->setEnabled( not (p->getExpander()->getEnabled()));
           p->getExpander()->setEnabled( not (p->getExpander()->getEnabled()));
        }
     }

}

void TTSaver::load_ttp_profile(Glib::ustring filename)
{
  std::ifstream myfile (filename);
  if (myfile.is_open())
  {
    resetFavoriteAndTrashState();
    themeSplitter(myfile);
    myfile.close();
  }
  else printf("Unable to open file: %s ",filename.c_str());
}

void TTSaver::save_ttp_profile(Glib::ustring filename)
{
  Glib::ustring lines = "";
//  printf("parsing toolPanels \n");

  std::vector<ToolPanel*> panels = env->getToolPanels();
  for (size_t i=0; i<panels.size(); i++)
  {
    FoldableToolPanel* p = static_cast<FoldableToolPanel*> (panels.at(i));
    if ((p != nullptr)
    && (!p->canBeIgnored()))
    { 
       lines += p->themeExport();
    }

  }

  std::ofstream myfile;
  myfile.open (filename);
  myfile << lines;
  myfile.close();
}

void TTSaver::autoload_clicked (GdkEventButton* event)
{
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
  enAutoloadSettingsLine->set_text(fname);
  options.save();
}

void TTSaver::save_clicked (GdkEventButton* event)
{

    if (event->button != 1) {
        return;
    }

    Gtk::FileChooserDialog dialog (getToplevelWindow (this), M("PROFILEPANEL_SAVEDLGLABEL"), Gtk::FILE_CHOOSER_ACTION_SAVE);

    Glib::ustring filename =  profilbox->get_active_text() + paramFileGuiExtension;

    dialog.set_current_name (filename);
    dialog.set_current_folder (options.getUserProfilePath()) ; 

    //Add response buttons the the dialog:
    dialog.add_button(Gtk::StockID("gtk-cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::StockID("gtk-save"), Gtk::RESPONSE_OK);

    //Add filters, so that only certain file types can be selected:

    Glib::RefPtr<Gtk::FileFilter> filter_pp = Gtk::FileFilter::create();
    filter_pp->set_name(M("FILECHOOSER_FILTER_TTP"));
    filter_pp->add_pattern("*" + paramFileGuiExtension);
    dialog.add_filter(filter_pp);

    Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
    filter_any->set_name(M("FILECHOOSER_FILTER_ANY"));
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);

//    dialog.set_do_overwrite_confirmation (true);

    bool done = false;

    do 
    {
      if (dialog.run() == Gtk::RESPONSE_OK) 
      {
        std::string fname = dialog.get_filename();
        Glib::ustring ext = getExtension (fname);

        if (("." + ext) != paramFileGuiExtension) 
        {
          fname += paramFileGuiExtension;
        }

        if (!confirmOverwrite (dialog, fname)) 
        {
          continue;
        }

        Glib::ustring lastFilename = Glib::path_get_basename (fname);
        save_ttp_profile(fname);

        size_t lastdot = lastFilename.find_last_of ('.');
        Glib::ustring name = lastFilename.substr(0, lastdot);
        
        if ( std::find(entries.begin(),entries.end(),fname) == entries.end() )
        {
          printf("name=[%s]\n",name.c_str());
          printf("fname=[%s]\n",fname.c_str());

          profilbox->append(name);
          entries.push_back(fname);
        }

        done = true;
      }
      else done = true;
    } while (!done);
    return;
}


Glib::ustring TTSaver::themeExport()
{
  Glib::ustring favSettings = getToolName() + ":"  + "favorite:";
  Glib::ustring oriSettings = getToolName() + ":"  + "original:";
  Glib::ustring traSettings = getToolName() + ":"  + "trash:";

  std::vector<ToolPanel*> panels = env->getToolPanels();

  std::sort (panels.begin(), panels.end(), sortByFav);

  for (size_t i=0; i<panels.size(); i++)
  {
    FoldableToolPanel* p = static_cast<FoldableToolPanel*> (panels.at(i));
    if ((p != nullptr)
    && (!p->canBeIgnored()))
    {
//      int posFav = p->getPosOri();
      if (p->getFavoriteButton()->get_active())
        favSettings += p->getToolName() + " ";
    }
  }

  std::sort (panels.begin(), panels.end(), sortByOri);

  for (size_t i=0; i<panels.size(); i++)
  {
    FoldableToolPanel* p = static_cast<FoldableToolPanel*> (panels.at(i));
    if ((p != nullptr)
    && (!p->canBeIgnored()))
    {
      int posOri = p->getOriginalBox()->getPos(p); // maybe todo getPos()
      if (!(p->getTrashButton()->get_active()))
         oriSettings += p->getToolName()+ "(" + p->getOriginalBox()->getBoxName() + ":" + IntToString(posOri) + ") ";

      if (p->getTrashButton()->get_active())
        traSettings += p->getToolName() + " ";
    }
  }

  return favSettings + "\n" +  oriSettings + "\n" + traSettings +"\n"; 
}

void TTSaver::themeImport(std::ifstream& myfile)
{

  std::vector<std::string> favoriteItems;
  std::vector<std::string> normalItems;
  std::vector<std::string> trashItems;

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
   //     printf("%s\n", token.c_str());
      if (token == getToolName())
      {
        if (getline(tokensplitter, token, ':'))
        {
          if (token == "favorite")
          {
            while(getline(tokensplitter, token, ' '))
            { 
//              printf("f(%s)\n", token.c_str());
              favoriteItems.push_back(token);
            }
          }

          if (token == "original")
          {
            while(getline(tokensplitter, token, ' '))
            {
//              printf("(%s)\n", token.c_str());
              normalItems.push_back(token);
            }
          }

          if (token == "trash")
          { 
            while(getline(tokensplitter, token, ' '))
            {
//              printf("t(%s)\n", token.c_str());
              trashItems.push_back(token);
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


  // putting the panels into a map 
  std::vector<ToolPanel*> panels = env->getToolPanels();
  std::map<std::string,ToolPanel*> map;
  for (size_t i=0; i<panels.size(); i++)
  {
    if (!panels.at(i)->canBeIgnored())
    {
      map[panels.at(i)->getToolName()] = panels.at(i);
    }
  }
 
 // putting the container tabs (vbox) into a map 
 std::vector<ToolVBox*> boxList = env->getVBoxList();
 std::map<std::string,ToolVBox*> mapVBox;
 for (size_t i=0; i<boxList.size(); i++)
  {
    mapVBox[boxList.at(i)->getBoxName()] = boxList.at(i);
  }

  // initializing normal status, position and parent to attach the filter to
  for (size_t i=0; i<normalItems.size(); i++)
  {
    std::istringstream tokensplitter(normalItems.at(i));
    std::string fname;
    std::string vname;
    std::string spos;
    if (getline(tokensplitter, fname, '('))
    {
 //     printf("act on:%s\n", fname.c_str());
      
      if (getline(tokensplitter, vname, ':'))
      {
//        printf("of: :%s\n", vname.c_str());

        if (getline(tokensplitter, spos, ')'))
        {
          int pos =  atoi(spos.c_str());
//          printf("pos: :%i\n", pos);
          // if a panel is not defined anymore.
          // either the name got changed or the user used an older version.
          if ((map[fname]) && (mapVBox[vname]))
          {
            mapVBox[vname]->addPanel(map[fname],pos);
            map[fname]->setOriginalBox(mapVBox[vname]);
            map[fname]->setPLocation(PANEL_ON_NORMAL);// 
            map[fname]->setPosOri(pos);
          }
        }
      }
    }
  }

  // initializing favorite status
  for (size_t i=0; i<favoriteItems.size(); i++)
  if (map[favoriteItems.at(i)])
  {
    printf("parsing panel=%s\n",map[favoriteItems.at(i)]->getToolName().c_str());
    map[favoriteItems.at(i)]->getFavoriteButton()->set_active(true);
 //   printf("favorite:%s at:%i \n",map[favoriteItems.at(i)]->getToolName().c_str(), i);
    if (map.count( map[favoriteItems.at(i)]->getOriginalBox()->getBoxName())>0)
    {
      //  hack : when the panel is in a collapsed container, 
      // it will not appear unless we collapse / uncollapse the container first.
      bool state = map.at(map[favoriteItems.at(i)]->getOriginalBox()->getBoxName())->getExpander()->get_expanded();
      map.at(map[favoriteItems.at(i)]->getOriginalBox()->getBoxName())->getExpander()->set_expanded(!state);  
      map.at(map[favoriteItems.at(i)]->getOriginalBox()->getBoxName())->getExpander()->set_expanded(state);
    }
// element from toolPanel will not be sorted out - because of side effects in the tt panel 
if (true) //map[favoriteItems.at(i)]->getOriginalBox()->getBoxName() == "favoritePanel" )
   {
    printf("Setting favorite pos=%zu for %s\n",i, map[favoriteItems.at(i)]->getToolName().c_str());
    env->setFavoritePos( map[favoriteItems.at(i)], i);
    map[favoriteItems.at(i)]->updateLabelInfo();
   }
  }

 // initializing trash status
  for (size_t i=0; i<trashItems.size(); i++)
  {
    if (map[trashItems.at(i)])
      map[trashItems.at(i)]->getTrashButton()->set_active(true);
  }

  // getting panels that were not displayed at all. could be caused by loading an other ttp profile coupled with cleanBox call.
  
  for (size_t i=0; i< env->getToolPanels().size() ; i++)
   {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (env->getPanel(i));
      if ( (p != nullptr)
      && (!(p->canBeIgnored()))
      && (p->getPLocation() == PANEL_ON_UNDEF))
      {
        if (options.rtSettings.verbose)
          printf("panel %s has a non correct location - it is probably absent from the ttp profile - correcting this\n", p->getToolName().c_str());
        p->moveToOriginal();
      }
  }


}
