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
#include "../rtengine/safegtk.h"
#include "rtimage.h"
#include <sstream>
#include <fstream>


using namespace rtengine;
using namespace rtengine::procparams;

TTSaver::TTSaver () : FoldableToolPanel(this,"ttsaver",M("TP_THEMETOOL_LABEL"),false,false)
{
	themeBox = Gtk::manage(new Gtk::HBox());
	themeBox->set_spacing(4);
	themeFile = Gtk::manage(new MyFileChooserButton(M("TP_THEME_LABEL"), Gtk::FILE_CHOOSER_ACTION_OPEN));
	themeFilePersister.reset(new FileChooserLastFolderPersister(themeFile, options.lastTTSaverDir));
	themeLabel = Gtk::manage(new Gtk::Label(M("GENERAL_FILE")));
	buttonReset = Gtk::manage(new Gtk::Button());
	buttonReset->set_image (*Gtk::manage(new RTImage ("gtk-cancel.png")));
        buttonSave = Gtk::manage(new Gtk::Button());
        buttonSave->set_image (*Gtk::manage(new RTImage ("saved.png")));

	themeBox->pack_start(*themeLabel, Gtk::PACK_SHRINK, 0);
	themeBox->pack_start(*themeFile);
	themeBox->pack_start(*buttonReset, Gtk::PACK_SHRINK, 0);
        themeBox->pack_start(*buttonSave, Gtk::PACK_SHRINK, 0);
     
	pack_start( *themeBox, Gtk::PACK_SHRINK, 0);
	buttonReset->signal_clicked().connect( sigc::mem_fun(*this, &TTSaver::themeReset), true );
        s = buttonSave->signal_clicked().connect( sigc::mem_fun(*this, &TTSaver::themeSave), true );

	
	// Set filename filters
	Gtk::FileFilter *filter_any = Gtk::manage(new Gtk::FileFilter);
        filter_any->add_pattern("*");
        filter_any->set_name(M("TP_FLATFIELD_FILEDLGFILTERANY"));
        themeFile->add_filter (*filter_any);
    
    // filters for all supported non-raw extensions       
    for (size_t i=0; i<options.parseExtensions.size(); i++) {
        if (options.parseExtensionsEnabled[i] && options.parseExtensions[i].uppercase()!="THE" )
        {    
	        Gtk::FileFilter *filter_tt = Gtk::manage(new Gtk::FileFilter);
	        filter_tt->add_pattern("*." + options.parseExtensions[i]);
   	        filter_tt->add_pattern("*." + options.parseExtensions[i].uppercase());
	        filter_tt->set_name(options.parseExtensions[i].uppercase());
	        themeFile->add_filter (*filter_tt);
	        //printf("adding filter %s \n",options.parseExtensions[i].uppercase().c_str());
        }
    }
}

bool sortByFav(ToolPanel* t1, ToolPanel* t2)
{
  if (t1->canBeIgnored()
  || t2->canBeIgnored()) return false;

  return (t1->getPosFav() < t2->getPosFav());
}


bool sortByOri(ToolPanel* t1, ToolPanel* t2)
{
  if (t1->canBeIgnored()
  || t2->canBeIgnored()) return false;
  
  
  if (t1->getOriginalBox()->getBoxName() == t2->getOriginalBox()->getBoxName())
    return (t1->getPosOri() < t2->getPosOri());
  else return (t1->getOriginalBox()->getBoxName() < t2->getOriginalBox()->getBoxName());
}

void TTSaver::resetFavoriteAndTrashState() 
{
  std::vector<ToolPanel*> panels = env->getPanels();
  for (size_t i=0; i<panels.size(); i++)
  {
    if (!panels.at(i)->canBeIgnored())
    {
      // resetting favorite and trash status
      panels.at(i)->getFavoriteButton()->set_active(false);
      panels.at(i)->getTrashButton()->set_active(false);
    }
  }

}

void TTSaver::themeImport(Glib::ustring lines)
{

  env->disableSwitchPageReaction = true;
  env->state = ENV_STATE_IN_NORM;
  env->prevState = ENV_STATE_IN_NORM;

  std::istringstream linesplitter(lines);
  std::string line;

  std::vector<std::string> favoriteItems;
  std::vector<std::string> normalItems;
  std::vector<std::string> trashItems;


  while (getline(linesplitter, line))
  {
    std::istringstream tokensplitter(line);
    std::string token;
    printf("parsing line: %s\n", line.c_str());

    if (getline(tokensplitter, token, ':'))
    {
 //     printf("%s\n", token.c_str());
      if (token == getName())
      {
        if (getline(tokensplitter, token, ':'))
        {
          if (token == "favorite")
          {
            while(getline(tokensplitter, token, ' '))
            { 
              printf("f(%s)\n", token.c_str());
              favoriteItems.push_back(token);
            }
          }

          if (token == "original")
          {
            while(getline(tokensplitter, token, ' '))
            {
              printf("(%s)\n", token.c_str());
              normalItems.push_back(token);
            }
          }

          if (token == "trash")
          { 
            while(getline(tokensplitter, token, ' '))
            {
              printf("t(%s)\n", token.c_str());
              trashItems.push_back(token);
            }
          }
        }
      }
    }
  }

  // puting the vector element into a map (toolpanel)
  std::vector<ToolPanel*> panels = env->getPanels();
  std::map<std::string,ToolPanel*> map;
  for (size_t i=0; i<panels.size(); i++)
  {
    if (!panels.at(i)->canBeIgnored())
    {
      map[panels.at(i)->getName()] = panels.at(i);
    }
  }
 
 // putting the vector element into a map (vbox)
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
      printf("act on:%s\n", fname.c_str());
      
      if (getline(tokensplitter, vname, ':'))
      {
        printf("of: :%s\n", vname.c_str());

        if (getline(tokensplitter, spos, ')'))
        {
          int pos =  atoi(spos.c_str());
          printf("pos: :%i\n", pos);
          env->reAttachPanel(map[fname], mapVBox[vname], pos);
        }
      }
    }
  }

  // initializing favorite status
  for (size_t i=0; i<favoriteItems.size(); i++)
  {
    map[favoriteItems.at(i)]->getFavoriteButton()->set_active(true);
    printf("favorite:%s at:%i \n",map[favoriteItems.at(i)]->getName().c_str(), i);
    if (map.count( map[favoriteItems.at(i)]->getOriginalBox()->getBoxName())>0)
    {
      //  hack : when the panel is in a collapsed container, 
      // it will not appear unless we collapse / uncollapse the container first.
      bool state = map.at(map[favoriteItems.at(i)]->getOriginalBox()->getBoxName())->getExpander()->get_expanded();
      map.at(map[favoriteItems.at(i)]->getOriginalBox()->getBoxName())->getExpander()->set_expanded(!state);  
      map.at(map[favoriteItems.at(i)]->getOriginalBox()->getBoxName())->getExpander()->set_expanded(state);
    }
    env->setFavoritePos( map[favoriteItems.at(i)], i);
  }

 // initializing trash status
  for (size_t i=0; i<trashItems.size(); i++)
  {
    map[trashItems.at(i)]->getTrashButton()->set_active(true);
  }

  env->disableSwitchPageReaction = false;

}

Glib::ustring TTSaver::themeExport()
{
  Glib::ustring favSettings = getName() + ":"  + "favorite:";
  Glib::ustring oriSettings = getName() + ":"  + "original:";
  Glib::ustring traSettings = getName() + ":"  + "trash:";

  std::vector<ToolPanel*> panels = env->getPanels();

  std::sort (panels.begin(), panels.end(), sortByFav);

  for (size_t i=0; i<panels.size(); i++)
  {
    FoldableToolPanel* p = static_cast<FoldableToolPanel*> (panels.at(i));
    if ((p != NULL)
    && (!p->canBeIgnored()))
    {
      int posFav = p->getPosOri();
      if (p->getFavoriteButton()->get_active())
        favSettings += p->getName() + " ";
    }
  }

  std::sort (panels.begin(), panels.end(), sortByOri);

  for (size_t i=0; i<panels.size(); i++)
  {
    FoldableToolPanel* p = static_cast<FoldableToolPanel*> (panels.at(i));
    if ((p != NULL)
    && (!p->canBeIgnored()))
    {
      int posOri = p->getPosOri();
      if (!(p->getTrashButton()->get_active()))
         oriSettings += p->getName()+ "(" + p->getOriginalBox()->getBoxName() + ":" + IntToString(posOri) + ") ";

      if (p->getTrashButton()->get_active())
        traSettings += p->getName() + " ";
    }
  }
  return favSettings + "\n" +  oriSettings + "\n" + traSettings + "\n";
}

void TTSaver::themeLoad()
{

}

void TTSaver::themeReset()
{
  std::string line;
  std::ifstream myfile ("/home/keby/myfile.ttp");

  resetFavoriteAndTrashState();
  if (myfile.is_open())
  {
    while ( getline (myfile,line) )
    {
      themeImport(line);
    }
    myfile.close();
  }

  else printf("Unable to open file");

}

void TTSaver::themeSave()
{
  Glib::ustring lines;
  printf("parsing toolPanels \n");
  lines = themeExport();
 // themeImport(lines);

 // printf("%s",  lines.c_str());

  std::ofstream myfile;
  myfile.open ("/home/keby/myfile.ttp");
  myfile << lines;
  myfile.close();
}

void TTSaver::test(Glib::ustring name)
{
  printf("button enbled clicked - %s \n", name.c_str());
}



