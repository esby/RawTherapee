/*
 *  This file is part of RawTherapee.
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

// esby fork: ToolPanelCoordinator methods added by the fork (favorites, tool moves,
// useful/trash tabs, environment variables). They were moved from toolpanelcoord.cc
// unchanged, so toolpanelcoord.cc only keeps the upstream code and a few hooks.

#include <iostream>

#include "multilangmgr.h"
#include "ttlog.h"
#include "toolpanelcoord.h"
#include "metadatapanel.h"
#include "options.h"
#include "rtimage.h"

#include "../rtengine/imagesource.h"
#include "../rtengine/dfmanager.h"
#include "../rtengine/ffmanager.h"
#include "../rtengine/improcfun.h"
#include "../rtengine/perspectivecorrection.h"
#include "../rtengine/procevents.h"
#include "../rtengine/refreshmap.h"
//#include "../rtexif/rtexif.h"
#include "ttdep.h"
#include "../rtengine/metadata.h"

using namespace rtengine::procparams;

using Tool = ToolPanelCoordinator::Tool;
using ToolTree = ToolPanelCoordinator::ToolTree;

void ToolPanelCoordinator::addfavoritePanel (Gtk::Box* where, FoldableToolPanel* panel, int level)
{
  if (useRtFav){
    auto name = panel->getToolName();
    auto it = std::find(options.favorites.begin(), options.favorites.end(), name);
    if (it != options.favorites.end()) {
        int index = std::distance(options.favorites.begin(), it);
        favorites[index] = panel;
    } else {
        addPanel(where, panel, level);
    }
  }
  else
    addPanel(where, panel, level);
}

void ToolPanelCoordinator::doDeploy()
{
    for(auto toolPanel : toolPanels) 
    {
//      printf("panel nb=%i \n",  i);
        toolPanel->deploy();
    }
}

void ToolPanelCoordinator::doDeployLate()
{
//    if ( options.rtSettings.verbose )
      TT_LOG("late panel deployment for env=#%i \n", env->getEnvRef());
    for(auto toolPanel : toolPanels) 
    {   
//      printf("panel nb=%i \n",  i);
      toolPanel->deployLate();
    }
    env->disableSwitchPageReaction = false;

}

void ToolPanelCoordinator::doReact(FakeProcEvent ev)
{
    if ( options.rtSettings.verbose ) 
      printf("enabling panel reac for envId=%i\n", env->getId());

    if (!isReaction)
    {
      isReaction = true;

      for(auto toolPanel : toolPanels) 
        toolPanel->setReacted(false);

      for(auto toolPanel : toolPanels) 
      {
//      printf("panel nb=%i \n",  i);
        if (!toolPanel->getReacted())
        {
          toolPanel->setReacted(true);
          toolPanel->react(ev);
        }
      }
      isReaction = false;
    }
}

void ToolPanelCoordinator::handlePanel(Gtk::VBox* vbox, Gtk::ScrolledWindow* panelSW, int panelIterator, int spacing) {
   panelSW->add(*vbox);
   vbox->pack_start(*hsPanelEnd[panelIterator], Gtk::PACK_SHRINK,0);
   vbox->pack_start(*vbPanelEnd[panelIterator], Gtk::PACK_SHRINK,spacing);
   vbPanel[panelIterator] = vbox;
   ToolVBox* box =  static_cast<ToolVBox*>(vbox);
   box->setParent(toolPanelNotebook);
   box->setParentSW(panelSW);
}

//todo refactor
/*
void parseDirectory(rtexif::TagDirectory* d, Glib::ustring prefix, Environment* env)
{
  if (d)
    for(int i = 0; i < d->getCount(); ++i)
    {
       rtexif::Tag* t = d->getTagByIndex(i);

       Glib::ustring prefixedName = prefix + ':' + t->nameToString();
       if (prefix=="")
         prefixedName = t->nameToString();

//       printf("trying to parse tag name=%s:%s \n",prefixedName.c_str(), t->nameToString().c_str());

       if (t->getDirectory())
       {
//         printf("parsing directory= %s \n",prefixedName.c_str());
         parseDirectory(t->getDirectory(),prefixedName,env);
       }
       else
       {
//         printf("... value=%s \n",t->valueToString().c_str());
         env->setVar(prefixedName, t->valueToString());
       }
   }
}*/

// rtexif was removed upstream, the exif data is now read through Exiv2.
// the variable names follow the old rtexif tree, so the existing names keep working:
//   Exif.Image.Make            -> Make
//   Exif.Photo.ExposureTime    -> Exif:ExposureTime
//   Exif.<maker group>.Tag     -> Exif:MakerNote:Tag   (ex: Exif.Panasonic.RollAngle)
//   Exif.Iop.Tag               -> Exif:Interoperability:Tag
//   Exif.<other group>.Tag     -> <other group>:Tag    (ex: GPSInfo:GPSLatitude)
static Glib::ustring exifVariableName(const Exiv2::Exifdatum& datum)
{
    const std::string group = datum.groupName();
    const std::string tag = datum.tagName();

    if (group == "Image")
        return tag;
    if (group == "Photo")
        return "Exif:" + tag;
    if (group == "Iop")
        return "Exif:Interoperability:" + tag;
    if (Exiv2::ExifTags::isMakerGroup(group))
        return "Exif:MakerNote:" + tag;
    return group + ":" + tag;
}

static void transmitExifData(const Glib::ustring& fname, Environment* env)
{
    // values of the previous image must not stay visible
    env->clearVarsWithPrefix(ROOT_EXIF_PREFIX + ":");

    try
    {
        rtengine::Exiv2Metadata meta(fname);
        meta.load();
        const Exiv2::ExifData& exifData = meta.exifData();
        int count = 0;

        for (const auto& datum : exifData)
        {
            if (datum.size() > 512) // binary blobs (thumbnails, unknown makernote data...)
                continue;

            // offsets to the sub-directories, they were not part of the rtexif tree
            const std::string tag = datum.tagName();
            if ((tag == "ExifTag") || (tag == "GPSTag") || (tag == "InteroperabilityTag"))
                continue;

            // tags unknown to Exiv2 are named after their number (ex: 0x002c), their value
            // cannot be interpreted and is mostly a long list of numbers: they are skipped.
            if (tag.compare(0, 2, "0x") == 0)
                continue;

            // print() gives the interpreted value, as rtexif valueToString() did
            Glib::ustring value(datum.print(&exifData));
            if (!value.validate()) // not valid utf-8, it could not be displayed
                continue;

            env->setVar(ROOT_EXIF_PREFIX + ":" + exifVariableName(datum), value);
            count++;
        }

        if (options.rtSettings.verbose)
            printf("%i exif values transmitted by variables\n", count);
    }
    catch (const std::exception& e) // Exiv2::Error derives from std::exception
    {
        printf("unable to transmit the exif data of %s: %s\n", fname.c_str(), e.what());
    }
}

void ToolPanelCoordinator::on_notebook_switch_page(Gtk::Widget* /* page */, guint page_num){
  if (options.rtSettings.verbose)
    printf(" on_notebook_switch_page called\n");
  if (!env->disableSwitchPageReaction)
    {

    env->prevState = env->state;
    if (options.rtSettings.verbose)     
      printf("notebook switch page %c-> ", env->prevState);
    if (toolPanelNotebook->get_current_page() == toolPanelNotebook->page_num(*favoritePanelSW))
    {
       env->state = ENV_STATE_IN_FAV;      
       if (options.rtSettings.verbose)
         printf("%c -> favorite panel\n", env->state);
    }
    else 
    if (toolPanelNotebook->get_current_page() == toolPanelNotebook->page_num(*trashPanelSW)) 
    {
       env->state = ENV_STATE_IN_TRASH;
       if (options.rtSettings.verbose)
         printf("%c -> trash panel\n", env->state);
    }else
    {
      env->state = ENV_STATE_IN_NORM;
      if (options.rtSettings.verbose)
        printf("%c -> normal panel\n", env->state);
    }

   // the positions are saved from the tab we are leaving, before the panels are moved.
   // this keeps the moves done by the user (up/down/left/right), otherwise moveToOriginal()
   // and moveToFavorite() would put the panels back to the positions loaded from the ttp profile.
   // note: positions are only reliable in the state we leave:
   // - normal tabs: every panel not in the trash is in its original box.
   // - favorite tab: every favorite panel is in the favorite box.
   savePanelPositions(env->prevState);

   // we only checks outside of fav <> fav or trash <> trash interactions
   if (!((env->state == env->prevState)
    &&(env->state != ENV_STATE_IN_NORM)))
   {

     // todo: fix the performance issue.
     // most of the time is spend on this loop
     // there is probably an optimization that needs to be performed at some point.

   // dc determine the tabs that was displayed before the tab switch 
    // 11 for favorite to favorite tabs.
    // 12 for favorite to normal tabs.
    // 13 for favorite to trash tabs.
    // 21 for normal to favorite tabs.
    // 22 n -> n
    // 23 n -> t
    // 31 t -> f
    // 32 t -> n
    // 33 t -> t

    int dc=0;
    if (env->prevState == ENV_STATE_IN_FAV) dc+= 10;
    if (env->prevState == ENV_STATE_IN_TRASH) dc+= 30;
    if (env->prevState == ENV_STATE_IN_NORM) dc+= 20;

    if (env->state == ENV_STATE_IN_FAV) dc+= 1;
    if (env->state == ENV_STATE_IN_TRASH) dc+= 3;
    if (env->state == ENV_STATE_IN_NORM) dc+= 2;

// done: this was splitted into three parts
// as it was causing positionning issues with favorite / normal panels, with position not loaded correctly
/*
    for(auto toolPanel : toolPanels) 
        toolPanel->favorite_others_tabs_switch(dc);
*/

    // first: favorite
    // then each normal panel in order
    // then trash panel

    // handling favorite panels
    std::vector<ToolPanel*> panels = env->getToolPanels();

    std::stable_sort (panels.begin(), panels.end(), sortByFav);

    for (std::vector<ToolPanel*>::iterator it = panels.begin() ; it != panels.end(); ++it)
    {
     ToolPanel* p = static_cast<ToolPanel*>(*it);  

      if (p->getFavoriteButton()->get_active() == true)
      {
//        printf("Parsing VBox switch todo FAV %s pos=%i \n", p->getToolName().c_str(), p->getPosFav());
        p->favorite_others_tabs_switch(dc);
      }
    }

    panels = env->getToolPanels();
    std::stable_sort (panels.begin(), panels.end(), sortByFav);
 
    /* - unused debug code
    for (std::vector<ToolPanel*>::iterator it1 = panels.begin() ; it1 != panels.end(); ++it1)
    {
      ToolPanel* p1 = static_cast<ToolPanel*>(*it1);
//      printf("%s.getPLocation= %i \n",p1->getToolName().c_str(),p1->getPLocation());

      for (std::vector<ToolPanel*>::iterator it2 = it1+1 ; it2 != panels.end(); ++it2)
      {
        ToolPanel* p2 = static_cast<ToolPanel*>(*it2);
        if ((p1->getFavoriteButton()->get_active() == true)
        && (p2->getFavoriteButton()->get_active() == true))
        {
          int pp1 = favoritePanel->getPos(p1);
          int pp2 = favoritePanel->getPos(p2);
 
//      printf("A: %s.ps %i  vs %s.ps %i \n", p1->getToolName().c_str(), pp1, p2->getToolName().c_str(), pp2);
          if (pp1 > pp2)
          {
             printf("Anomaly: %s.getPosFav() %i  > %s.getPosFav() %i \n", p1->getToolName().c_str(), p1->getPosFav(), p2->getToolName().c_str(), p2->getPosFav());

          }
        }
      }
    }
    */


    // second part normal panels are handled
    panels = env->getToolPanels();

    std::stable_sort (panels.begin(), panels.end(), sortByOri);

    for (std::vector<ToolPanel*>::iterator it = panels.begin() ; it != panels.end(); ++it)
    {
     ToolPanel* p = static_cast<ToolPanel*>(*it);

      if ((p->getFavoriteButton()->get_active() == false)
         && (p->getTrashButton()->get_active() == false))
      {
//        printf("Parsing VBox switch todo NORM %s \n", p->getToolName().c_str());
        p->favorite_others_tabs_switch(dc);
      }
    }

    // third part trash panels
    for (std::vector<ToolPanel*>::iterator it = panels.begin() ; it != panels.end(); ++it)
    {
     ToolPanel* p = static_cast<ToolPanel*>(*it);

      if (p->getTrashButton()->get_active() == true)
      {
//        printf("Parsing VBox switch todo TRASH %s \n", p->getToolName().c_str());
        p->favorite_others_tabs_switch(dc);
      }
    }

/*
    for(auto toolPanel : toolPanels) 
        toolPanel->favorite_others_tabs_switch(dc);
*/

    //putting the ending panels and separator to the end
    // note: pos was larger than the number of children, so both widgets were moved to the end
    // in that order, the separator ending up after the ornament. -1 means "at the end".
    for(int i=0; i< NB_PANEL; i++){
//      int pos = toolPanels.size()-1;
//      Gtk::Widget* w = static_cast<Gtk::Widget*>(vbPanelEnd[i]);
//      vbPanel[i]->reorder_child(*w, pos);
//      w =(Gtk::Widget*)hsPanelEnd[i];
//      vbPanel[i]->reorder_child(*w, pos-1);
      vbPanel[i]->reorder_child(*hsPanelEnd[i], -1);
      vbPanel[i]->reorder_child(*vbPanelEnd[i], -1);
     }
   }
    // we update label info all the time
    // updating the label info(currently the position number)
    for(auto toolPanel : toolPanels) 
        toolPanel->updateLabelInfo();
  }
}

// registers every tool of the upstream table in its tab, and the sub-tools in the
// sub-tools container of their parent tool. a sub-tools container is set up as a box
// of its own (named after its tool), so its sub-tools can be moved out of it.
void ToolPanelCoordinator::registerToolsFromLayout()
{
    // fixed order: the registration order is the order of the expanded states saved in options
    const std::vector<std::pair<Panel, ToolVBox*>> panels = {
        {Panel::EXPOSURE, exposurePanel},
        {Panel::DETAILS, detailsPanel},
        {Panel::COLOR, colorPanel},
        {Panel::ADVANCED, advancedPanel},
        {Panel::LOCALLAB, locallabPanel},
        {Panel::TRANSFORM_PANEL, transformPanel},
        {Panel::RAW, rawPanel},
    };

    std::function<void(Gtk::Box*, const std::vector<ToolTree>&, int)> registerTools =
        [&](Gtk::Box* box, const std::vector<ToolTree>& tools, int level)
    {
        for (const auto& tool : tools)
        {
            FoldableToolPanel* panel = getFoldableToolPanel(tool);
            if (panel == nullptr)
                continue;
            addfavoritePanel(box, panel, level);

            if (!tool.children.empty())
            {
                ToolVBox* subBox = static_cast<ToolVBox*>(static_cast<Gtk::Box*>(panel->getSubToolsContainer()));
                subBox->setBoxName(panel->getToolName());
                subBox->setPrevBox(static_cast<ToolVBox*>(box));
                subBox->setNextBox(static_cast<ToolVBox*>(box));
                env->addVBox(subBox);
                registerTools(panel->getSubToolsContainer(), tool.children, level + 1);
            }
        }
    };

    const ToolLayout& layout = getDefaultToolLayout();
    for (const auto& panel : panels)
    {
        auto it = layout.find(panel.first);
        if (it != layout.end())
            registerTools(panel.second, it->second, 1);
    }
}

// links the boxes used by moveLeft/moveRight, in the order of the notebook tabs.
// favorite and trash tabs are not part of the ring, nor tabs absent from the notebook (locallab in batch mode).
void ToolPanelCoordinator::linkPanelRing()
{
    std::vector<ToolVBox*> ring;
    for (int page = 0; page < toolPanelNotebook->get_n_pages(); page++)
    {
        Gtk::Widget* w = toolPanelNotebook->get_nth_page(page);
        for (int i = PANEL_SWITCHABLE_START; i < PANEL_SWITCHABLE_START + NB_PANEL_SWITCHABLE; i++)
        {
            ToolVBox* box = static_cast<ToolVBox*>(vbPanel[i]);
            if ((box != trashPanel) && (box->getParentSW() == w))
            {
                ring.push_back(box);
                break;
            }
        }
    }

    const size_t n = ring.size();
    for (size_t i = 0; i < n; i++)
    {
        ring[i]->setNextBox(ring[(i + 1) % n]);
        ring[(i + 1) % n]->setPrevBox(ring[i]);
    }

    if (options.rtSettings.verbose)
    {
        printf("panel ring:");
        for (auto box : ring)
            printf(" %s", box->getBoxName().c_str());
        printf("\n");
    }
}

void ToolPanelCoordinator::savePanelPositions(char fromState)
{
    for (auto p : env->getToolPanels())
    {
        if (p->canBeIgnored())
            continue;

        if (fromState == ENV_STATE_IN_NORM)
        {
            if (p->getOriginalBox() == nullptr)
                continue;
            int pos = p->getOriginalBox()->getPos(p);
            if (pos > -1)
                p->setPosOri(pos);
        }
        else if (fromState == ENV_STATE_IN_FAV)
        {
            int pos = favoritePanel->getPos(p);
            if (pos > -1)
                p->setPosFav(pos);
        }
    }
}

// replaces the upstream favorite check, which relied on the 'favorites' vector (unused with the esby favorites).
// returns the page where the tool is displayed, nullptr if the tool is inside a sub-block (lensgeom, ...).
Gtk::Widget* ToolPanelCoordinator::getToolPage(FoldableToolPanel* tool)
{
    if (tool->getFavoriteButton()->get_active())
        return favoritePanelSW;
    if (tool->getTrashButton()->get_active())
        return trashPanelSW;

    ToolVBox* box = tool->getOriginalBox();
    if ((box == nullptr) || (box->getParentSW() == nullptr))
        return nullptr;
    return box->getParentSW();
}

// scrolls the current tab so that the tool is displayed at the top.
// this is done in a short timeout: the tool may just have been moved by on_notebook_switch_page(),
// and its position is only known once GTK has recomputed the layout. An idle callback is not enough:
// the layout is driven by the frame clock and can be delayed until the next frame.
// fallback is used when the tool itself is not displayed (ex: rotate inside a folded lensgeom).
void ToolPanelCoordinator::scrollToTool(FoldableToolPanel* tool, FoldableToolPanel* fallback)
{
    Glib::signal_timeout().connect_once([this, tool, fallback]() {
        Gtk::ScrolledWindow* sw = dynamic_cast<Gtk::ScrolledWindow*>(
            toolPanelNotebook->get_nth_page(toolPanelNotebook->get_current_page()));
        if (sw == nullptr)
            return;

        // the ToolVBox is wrapped in a Gtk::Viewport by Gtk::ScrolledWindow::add()
        Gtk::Widget* content = sw->get_child();
        Gtk::Bin* viewport = dynamic_cast<Gtk::Bin*>(content);
        if ((viewport != nullptr) && (viewport->get_child() != nullptr))
            content = viewport->get_child();
        if (content == nullptr)
            return;

        MyExpander* exp = tool->getExpander();
        if (((exp == nullptr) || !exp->get_mapped()) && (fallback != nullptr))
            exp = fallback->getExpander();
        if ((exp == nullptr) || !exp->get_mapped())
            return;

        int x, y;
        if (exp->translate_coordinates(*content, 0, 0, x, y))
            sw->get_vadjustment()->set_value(y); // the adjustment clamps the value itself
    }, 50);
}

// transmits the image data (exif, size...) to the esby tools through the environment variables.
// called by initImage(): this code was moved from it.
void ToolPanelCoordinator::esbyTransmitImageData(const rtengine::FramesMetaData* pMetaData)
{
       const rtengine::FramesMetaData* idata = ipc->getInitialImage()->getMetaData();
       if( options.rtSettings.verbose ) 
       printf("transmiting some data via rtvar \n");
       //todo add idata to rtvar?
        env->setVar("Iso", idata->getISOSpeed());
        env->setVar("Fnum",  Glib::ustring(idata->apertureToString(idata->getFNumber())));
        env->setVar("Speed", Glib::ustring(idata->shutterToString(idata->getShutterSpeed())));
        env->setVar("FLen", idata->getFocalLen());
        env->setVar("Ecomp", Glib::ustring(idata->expcompToString(idata->getExpComp(),true)));
        env->setVar("Lens", idata->getLens());
        env->setVar("Camera", idata->getCamera());
//        env->setVar("Fname", openThm->getFileName());
//todo search another way of loading it
        env->setVar("Width", ipc->getFullWidth());
        env->setVar("Height", ipc->getFullHeight());
        // note: we will react later, on pp3 version tranmission
        TT_LOG("partial exif values transmitted by variables \n");

        TT_LOG("transmitting full exif data via rtvar \n");
        if (pMetaData->hasExif ()) 
        {
          //todo recuperates exifdata
          // Exiv2::ExifData exifdata = image_->exifData();
 //         rtexif::TagDirectory* root = pMetaData->getRootExifData() ;
   //       parseDirectory(root,ROOT_EXIF_PREFIX,env);
          transmitExifData(pMetaData->getFileName(), env);
          TT_LOG("full exif values tranmitted by variables \n");
          doReact(FakeEvFullExifTransmitted);
        }
}
