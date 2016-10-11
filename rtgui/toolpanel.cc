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
#include "toolpanel.h"
#include "toolpanelcoord.h"
#include "guiutils.h"

using namespace rtengine::procparams;


// Environment implementation
// todo: use another faster implementation relying on a map.
ToolPanel* Environment::getPanel(Glib::ustring name)
{
  printf("call to a potentially slow method : getPanel of Environement\n");
  for (size_t i=0; i<toolPanels.size(); i++)
   {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (toolPanels[i]);
      if (p != NULL)
        if (p->getToolName() == name)
          return (ToolPanel*)p;
   }
  return NULL;
}

ToolPanel* Environment::getPanel(int pos)
{
  if ((pos > -1)
  && (pos < toolPanels.size()))
  {
    FoldableToolPanel* p = static_cast<FoldableToolPanel*> (toolPanels[pos]);
    return p;
  }
  return NULL;
}

std::vector<ToolPanel*>::iterator   Environment::panelBegin()
{
  return toolPanels.begin();
}

void Environment::erasePanel (std::vector<ToolPanel*>::iterator it)   
{
  toolPanels.erase(it);
}


std::vector<ToolPanel*>::iterator   Environment::panelEnd()
{
  return toolPanels.end();
}

int Environment::countPanel()
{
  return toolPanels.size();
}

void  Environment::panelPushBack(ToolPanel* p)
{
  toolPanels.push_back(p);
}

MyExpander* Environment::getExpander(int pos)
{
  return expList.at(pos);
}

int Environment::countExpander()
{
  return expList.size();
}

void Environment::setFavoritePanel(ToolVBox* p)
{
  favoritePanel = p;
}

void Environment::setTrashPanel(ToolVBox* p)
{
  trashPanel = p;
}

void Environment::registerPanel(Gtk::Box* where, ToolPanel* panel)
{
  panel->setParent(where);
  expList.push_back (panel->getExpander());
  where->pack_start(*panel->getExpander(), false, false);
  panel->initVBox ((ToolVBox*)where, favoritePanel, trashPanel, this);
  toolPanels.push_back(panel);
}

void Environment::addVBox(ToolVBox* box) 
{ 
  boxList.push_back(box);
}

void Environment::reAttachPanel(ToolPanel *panel, ToolVBox* box, int pos)
{
//  printf("reattaching panel %s from %s to %s \n", panel->getToolName().c_str(), panel->getOriginalBox()->getBoxName().c_str(), box->getBoxName().c_str());
  Gtk::Container* c = (Gtk::Container*)box;
  panel->getOriginalBox()->remPanel(panel);
  box->addPanel(panel, pos);
}

void Environment::setFavoritePos(ToolPanel *panel, int pos)
{
  panel->getFavoriteBox()->remPanel(panel->getFavoriteDummy());
  panel->getOriginalBox()->remPanel(panel);
//  printf("panel:%s box:%s \n", panel->getToolName().c_str(), panel->getOriginalBox()->getBoxName().c_str());
  if (pos > -1)
    panel->getFavoriteBox()->addPanel(panel, pos);
}


ToolCounter::ToolCounter()
{

   box = static_cast<ToolVBox*> (this);
   prevBox = NULL;
   nextBox = NULL;
   boxName = "undefined";
}

int ToolCounter::size() {
             panelList = box->get_children ();
             return panelList.size ();
}

int ToolCounter::getPos(ToolPanel* panel) {

  panelList = box->get_children ();
  Gtk::Widget *w = panel->getExpander();
  auto it = std::find(panelList.begin(), panelList.end(), w);
  if (it == panelList.end())
    return -1;
  else
    return  std::distance(panelList.begin(), it);
}

ToolPanel* ToolCounter::getPanel(int pos) {
 panelList = box->get_children ();
 ToolPanel* p = (ToolPanel*) ((MyExpander*)panelList[pos])->getPanel();
   return p;
}

// we initiate the nextBox too sinec it's circular
void ToolCounter::setPrevBox(Gtk::VBox* _box) {
  prevBox = _box;
}


void ToolCounter::setNextBox(Gtk::VBox* _box) {
  nextBox = _box;
}

Gtk::VBox* ToolCounter::getPrevBox() {
  return prevBox;
}

Gtk::VBox* ToolCounter::getNextBox() {
  return nextBox;
}


void ToolCounter::remPanel(ToolPanel* t)
{
  ToolVBox* v = (ToolVBox*) this;
  MyExpander* exp = t->getExpander();
  int i = getPos(t);
  if (i>-1)
  {
//    printf("Removing panel: %s from: %s \n", t->getToolName().c_str(), this->getBoxName().c_str());
    v->remove(*exp);
  }
}


void ToolCounter::addPanel(ToolPanel* t, int pos)
{
  ToolVBox* v = (ToolVBox*) this;
  MyExpander* exp = t->getExpander();
  int i = getPos(t);
  if (i==-1)
  {
//    printf("adding Panel: %s to %s \n", t->getToolName().c_str(), this->getBoxName().c_str());
    v->pack_start(*exp, false,false);
    v->reorder_child(*exp, pos);
  }
}

ToolVBox::ToolVBox()
{
    updateStyle();
}

void ToolVBox::updateStyle()
{
    if (options.slimUI) {
        set_spacing(1);       // Vertical space between tools
        set_border_width(1);  // Space separating the tab's frame and the tools
    } else {
        set_spacing(2);       // Vertical space between tools
        set_border_width(1);  // Space separating the tab's frame and the tools  3
    }
}

void ToolVBox::on_style_changed (const Glib::RefPtr<Gtk::Style>& style)
{
    updateStyle();
}

ToolParamBlock::ToolParamBlock()
{
    updateStyle();
}

void ToolParamBlock::updateStyle()
{
    if (options.slimUI) {
        set_spacing(2);       // Vertical space between parameters in a single tool
        set_border_width(6);  // Space separating the parameters of a tool and its surrounding frame  6
    } else {
        set_spacing(4);       // Vertical space between parameters in a single tool
        set_border_width(8);  // Space separating the parameters of a tool and its surrounding frame  8
    }
}

void ToolParamBlock::on_style_changed (const Glib::RefPtr<Gtk::Style>& style)
{
    updateStyle();
}

void ToolPanel::updateLabelInfo() {
 if ((this->getExpander() != NULL)
  && ((!this->canBeIgnored()))) {

//    printf("toolName=%s \n",this->getToolName().c_str());

    int pos = originalBox->getPos(this);

    if (env->state == ENV_STATE_IN_FAV ) // favorite panel is visible
         pos = favoriteBox->getPos(this);

   if (env->state == ENV_STATE_IN_TRASH ) // favorite panel is visible
         pos = trashBox->getPos(this);

    char buffer[50];

/*
    int buttonChecked = 2;
    if (enabledButtonRef!=NULL){
      buttonChecked = 0;
      if (enabledButtonRef->get_active()) 
         buttonChecked = 1;
      }

    if (buttonChecked==2)
      sprintf(buffer, "%i [*]",pos);
    else
    if (buttonChecked==1)
      sprintf(buffer, "%i [✓]",pos);
    else
      sprintf(buffer, "%i [ ]",pos);
*/
    sprintf(buffer, "<tt>%2i</tt>", pos);
    this->labelInfo->set_markup(buffer); 
    this->labelInfoNotifier->activate();
  }

}

void ToolPanel::moveUp () {
  int pos;
  int npos;
  int count;
  ToolVBox* box;

  if (env->state == ENV_STATE_IN_FAV) {
//     printf("moveTop favorite - ");
     box = favoriteBox;
  }else {
//     printf("moveTop nopefav  - ");
     box = originalBox;
  }
  pos = box->getPos(this);
  npos = pos-1;
  count = box->size();

  if (npos > -1){
//     printf("- realized.\n");
     box->reorder_child(*getExpander(), npos); 
     updateLabelInfo();
     box->getPanel(pos)->updateLabelInfo(); // since we swapped it's pos and not npos
  }//else  printf("- canceled.\n");
}

void ToolPanel::moveDown () { 
  int pos;
  int npos;
  int count;
  ToolVBox* box;

  if (env->state == ENV_STATE_IN_FAV) {
//     printf("moveDown favorite - ");
     box = favoriteBox;
  }else {
//     printf("moveDown nopefav   - ");
     box = originalBox;
  }
  pos = box->getPos(this);
  npos = pos+1;
  count = box->size();

  if (npos < count-2){ // since there are two elements at the end...
//     printf("- realized.\n");
     box->reorder_child(*getExpander(), npos);     
     updateLabelInfo();
     box->getPanel(pos)->updateLabelInfo(); // since we swapped it's pos and not npos
  }//else  printf("- canceled.\n");

}

void ToolPanel::moveLeft() {
  ToolVBox* box =  originalBox;
  ToolVBox* nbox = (ToolVBox*) originalBox->getPrevBox();

  if (env->state == ENV_STATE_IN_NORM)
  {
//    printf("Moving %s from %s to %s \n", this->getToolName().c_str(), box->getBoxName().c_str(), nbox->getBoxName().c_str());

    //getting the page number where the object will be moved 
    Gtk::Notebook* notebook = (Gtk::Notebook*)nbox->getParent();
    int page_num = notebook->page_num(*nbox->getParentSW());

    //adjusting the scroll to the end     
    Gtk::ScrolledWindow* parentSw = (Gtk::ScrolledWindow*) (nbox->getParentSW());
    Gtk::Adjustment* adj = parentSw->get_vadjustment();
    if (!env->moveLeftToBottom)
      adj->set_value(adj->get_lower());
    else
      adj->set_value(adj->get_upper());

    box->remove(*this->getExpander());
    nbox->pack_start(*this->getExpander(), false,false);
   if (!env->moveLeftToBottom)
       nbox->reorder_child(*this->getExpander(), 0);             
    else
      nbox->reorder_child(*this->getExpander(), nbox->size()-1);

//    printf("page %i \n" , page_num);

    // the original box is changed
    this->originalBox = nbox;
    // updating panel infos
    int pos = nbox->getPos(this);
    nbox->getPanel(pos)->updateLabelInfo(); 
    notebook->set_current_page (page_num);
  }
}


//  moveLeftToBottom 
//       moveRightToTop 

void ToolPanel::moveRight() {
  ToolVBox* box =  originalBox;
  ToolVBox* nbox = (ToolVBox*) originalBox->getNextBox();

  if (env->state == ENV_STATE_IN_NORM)
  {
 //   printf("Moving %s from %s to %s \n", this->getToolName().c_str(), box->getBoxName().c_str(), nbox->getBoxName().c_str());

    //getting the page number where the object will be moved 
    Gtk::Notebook* notebook = (Gtk::Notebook*) nbox->getParent();
    int page_num = notebook->page_num(*nbox->getParentSW());
 
   //adjusting the scroll to the top     
    Gtk::ScrolledWindow* parentSw = (Gtk::ScrolledWindow*) (nbox->getParentSW());
    Gtk::Adjustment* adj = parentSw->get_vadjustment();
    if (env->moveRightToTop)
      adj->set_value(adj->get_lower());
    else 
      adj->set_value(adj->get_upper());

    box->remove(*this->getExpander());
    nbox->pack_start(*this->getExpander(), false,false);
    if (env->moveRightToTop)
       nbox->reorder_child(*this->getExpander(), 0);   
    else
      nbox->reorder_child(*this->getExpander(), nbox->size()-1);
      

//    printf("page %i \n" , page_num);

    // the original box is changed
    this->originalBox = nbox;
  // updating panel infos
    int pos = nbox->getPos(this);
    nbox->getPanel(pos)->updateLabelInfo();
    notebook->set_current_page (page_num);

  }
}

void ToolPanel::on_toggle_button_favorite() 
{

}

void ToolPanel::on_toggle_button_trash() {

}


void ToolPanel::cleanBox() {
  favoriteBox->remPanel(favoriteDummy);
  originalBox->remPanel(originalDummy);
  favoriteBox->remPanel(this);
  originalBox->remPanel(this);
  trashBox->remPanel(this);
}


void ToolPanel::moveToFavorite(int posFav, int posOri)
{
  cleanBox();
  originalBox->addPanel(originalDummy,posOri);
  favoriteBox->addPanel(this, posFav);
}

void ToolPanel::moveToOriginal(int posFav, int posOri)
{
  cleanBox();
  if (posFav > -1)
    favoriteBox->addPanel(favoriteDummy,posFav);
  originalBox->addPanel(this, posOri);
}

void ToolPanel::moveToTrash(int posFav, int posOri)
{
  cleanBox();
  if (posFav>-1)
    favoriteBox->addPanel(favoriteDummy,posFav);
  originalBox->addPanel(originalDummy,posOri);
  trashBox->addPanel(this, -1); // there is no pos saved for this one
}

int  ToolPanel::getPosOri()
{
  if (originalBox == NULL)
    return -1;
  int posOri = originalBox->getPos(this);
  if (posOri == -1) posOri = originalBox->getPos(originalDummy);
  if (posOri == -1) posOri = originalBox->size()-2;
  return posOri;
}

int  ToolPanel::getPosFav()
{
  if (favoriteBox == NULL)
    return -1;
  int posFav = favoriteBox->getPos(this);
  if (posFav == -1) posFav = favoriteBox->getPos(favoriteDummy);
  return posFav;
}

int  ToolPanel::getPosTra()
{
  if (trashBox == NULL) 
    return -1;
  int posTra = trashBox->getPos(this);
  return posTra;
}


void ToolPanel::favorite_others_tabs_switch() {

  if ((this->getExpander() != NULL) 
  && ((!this->canBeIgnored()))) {

    int dc=0;
    if (env->prevState == ENV_STATE_IN_FAV) dc+= 10;
    if (env->prevState == ENV_STATE_IN_TRASH) dc+= 30;
    if (env->prevState == ENV_STATE_IN_NORM) dc+= 20;

    if (env->state == ENV_STATE_IN_FAV) dc+= 1;
    if (env->state == ENV_STATE_IN_TRASH) dc+= 3;
    if (env->state == ENV_STATE_IN_NORM) dc+= 2;


    int posOri = getPosOri();
    int posFav = getPosFav();
    int posTra = getPosTra();
//    printf("dc=%i\n",dc);

    // handling all the possible cases
    if (dc == 11) // fav to fav
    { 
       //we do nothing
    }
    if (dc == 12) // Fav to normal
    {  
 //      favoriteBox->remPanel(this);
       if (favoriteButton->get_active())
       {
          if (trashButton->get_active())
          {
             moveToTrash(posFav,posOri);
          }
          else
          {
             moveToOriginal(posFav,posOri);
          }
       }
       else 
       { 
         if (trashButton->get_active())
         {
            moveToTrash(posFav,posOri);
         }
         else
         {
            moveToOriginal(posFav,posOri);
         }
       }
    }

    if (dc == 13)  // fav to trash
    {
        if (favoriteButton->get_active())
       {
          if(trashButton->get_active())
          {
             moveToTrash(posFav,posOri);
          }
          else
          {
             moveToOriginal(posFav,posOri);
          }
  
       }
       else
       {
        if(trashButton->get_active())
          {
             moveToTrash(posFav,posOri);
          }
          else
          {
             moveToOriginal(posFav,posOri);
          }
       }
    }

    if (dc == 21) // normal -> favorite
    {
    
       if (favoriteButton->get_active())
       {
         moveToFavorite(posFav,posOri);
       }
       else
       {
         moveToOriginal(posFav, posOri);
       }

    }
    if (dc == 22) // normal -> normal 
    { // 
      if (trashButton->get_active())
      {
         moveToTrash(posFav,posOri);
      }
      else 
      {
         moveToOriginal(posFav,posOri);
      } 
    }

    if (dc == 23) // normal -> trash
    { 
      if (trashButton->get_active())
      {
        moveToTrash(posFav,posOri);
      }
      else // resume check here
      {
        moveToOriginal(posFav, posOri);
      }

    }

     if (dc == 31) // trash -> favorite
    {
      
       if (trashButton->get_active())
       {
         if (favoriteButton->get_active())
         {
           moveToFavorite(posFav,posOri);
         }
         else
         {
           moveToOriginal(posFav,posOri);
         }
       }
       else // not trash
       {   
        if (favoriteButton->get_active())
         {
           moveToFavorite(posFav,posOri);
         }
         else
         {
           moveToOriginal(posFav,posOri);
         }
       }
    }
    if (dc == 32) // trash -> normal
    {

       if (!trashButton->get_active())
       {
         moveToOriginal(posFav,posOri);
       }
    }

    if (dc == 33) // trash -> trash - nothing to be done
    {

    }
  }
}

Glib::ustring IntToString(int iVal)
{
    std::ostringstream ssIn;
    ssIn << iVal;
    Glib::ustring strOut = ssIn.str();
    return strOut;
}

Glib::ustring ToolPanel::getThemeInfo() {
  Glib::ustring res;
  res = getToolName();
  res += "|" + originalBox->getBoxName();
  res += "|" + IntToString(originalBox->getPos(this));
  res += "=" ;
  if (favoriteButton->get_active())
     res += "F" + IntToString(favoriteBox->getPos(this));
  else 
  if (trashButton->get_active())
     res += "T";
  else
     res += "N";

  return res;
}



void ToolPanel::initVBox(ToolVBox* _originalBox, ToolVBox* _favoriteBox, ToolVBox* _trashBox, Environment* _env){
//     printf("initVBox for name=%s \n", getToolName().c_str());

     env = _env;
     originalDummy = Gtk::manage (new DummyToolPanel("normal_PosSaver_of_" + this->getToolName(), env));
     favoriteDummy = Gtk::manage (new DummyToolPanel("favorite_PosSaver_of_" + this->getToolName(), env));

     originalBox = _originalBox;
     favoriteBox = _favoriteBox;
     trashBox = _trashBox;

     ToolParamBlock* t =  dynamic_cast<ToolParamBlock*> (this);
     std::vector<Gtk::Widget*> panelList;
     panelList = t->get_children ();

     Gtk::Box* b = getFUDLRBox(); //// adynamic_cast<Gtk::Box*> (panelList.at(0)); // hldr Box.

     //enabledButtonRef = exp->
 //    return;
/*
     if (b!=NULL)
     {
       if (canBeEnabled())
  //       enabledButtonRef = Gtk::manage (new Gtk::CheckButton(M("GENERAL_ENABLED"))); 
       else
       { 
         //trying to find enabledButton (first CheckBoxbutton after the element added generically.
         int psize = panelList.size();
         if (psize>0)
         {
           printf("size=%i \n", psize);
           Gtk::Widget* w = panelList.at(0); // there used to be the fdlr box then a separator
           enabledButtonRef = dynamic_cast<Gtk::CheckButton*> (w);
           if (enabledButtonRef != NULL)
           {
             t->remove(*enabledButtonRef);              
             printf("  %s has an enabledButton... retrieving it...\n",this->getToolName().c_str());
           }
           else // case when a Gtk::HBox is present
           {    // this is a bit ugly, oce could just rewrite the filter concerned...
             Gtk::HBox* h = dynamic_cast<Gtk::HBox*> (w);
             if (h != NULL)
             { 
               std::vector<Gtk::Widget*> panelList2;
               panelList2 =  h->get_children ();
               int psize2 = panelList2.size();
               if (psize2>1)
               {
                 Gtk::Widget* w = panelList2.at(0);
                 Gtk::Widget* w2 = panelList2.at(1);
                 enabledButtonRef = dynamic_cast<Gtk::CheckButton*> (w);
                 if (enabledButtonRef != NULL)
                 {
                   h->remove(*w);
                   h->remove(*w2);
                   b->pack_end(*w2, Gtk::PACK_SHRINK, true,  0);
                 }
               }
             }
           }
         }
       }
       if (enabledButtonRef==NULL) // adding a button that cannot be checked
       {
         enabledButtonRef = Gtk::manage (new Gtk::CheckButton(M("GENERAL_ENABLED")));
         enabledButtonRef->set_sensitive(false);
         b->pack_start(*enabledButtonRef, Gtk::PACK_SHRINK,  0);
         b->reorder_child(*enabledButtonRef, 1);
       }
       else
       {
         Gtk::Box* b = getFUDLRBox(); //dynamic_cast<Gtk::Box*> (panelList.at(0)); // hldr Box.
         b->pack_start(*enabledButtonRef, Gtk::PACK_SHRINK, true, 0);
         b->reorder_child(*enabledButtonRef, 0);
         enabledButtonRef->signal_clicked().connect( sigc::mem_fun(*this, &FoldableToolPanel::updateLabelInfo));
       }
     }
 */

//     printf("name=%s  ", getToolName().c_str());
//     printf("positionOriginal=%i\n", originalBox->getPos(this));
     updateLabelInfo();

}

FoldableToolPanel::FoldableToolPanel(Gtk::Box* content, Glib::ustring toolName, Glib::ustring UILabel, bool need11, bool useEnabled) : ToolPanel(toolName, need11), parentContainer(NULL), exp(NULL), lastEnabled(true)
{
    if (!content) {
        return;
    }


        labelInfo = Gtk::manage (new Gtk::Label("Infos"));
        labelInfoNotifier = Gtk::manage (new Gtk::Button());

        //defining buttons
        favoriteButton = Gtk::manage (new Gtk::ToggleButton (M("GENERAL_STATE_FAVORITE")));
        trashButton = Gtk::manage (new Gtk::ToggleButton (M("GENERAL_STATE_TRASH")));

        moveUButton = Gtk::manage (new Gtk::Button (M("GENERAL_MOVE_UP")));
        moveDButton = Gtk::manage (new Gtk::Button (M("GENERAL_MOVE_DOWN")));
        moveRButton = Gtk::manage (new Gtk::Button (M("GENERAL_MOVE_RIGHT")));
        moveLButton = Gtk::manage (new Gtk::Button (M("GENERAL_MOVE_LEFT")));
        fudlrBox = Gtk::manage (new Gtk::HBox ());

        //move buttons listeners
        favoriteButton->signal_toggled().connect( sigc::mem_fun(*this, &FoldableToolPanel::on_toggle_button_favorite) );
        trashButton->signal_toggled().connect( sigc::mem_fun(*this, &FoldableToolPanel::on_toggle_button_trash) );

        moveUButton->signal_clicked().connect(sigc::mem_fun(*this, &FoldableToolPanel::moveUp));
        moveDButton->signal_clicked().connect(sigc::mem_fun(*this, &FoldableToolPanel::moveDown));
        moveRButton->signal_clicked().connect(sigc::mem_fun(*this, &FoldableToolPanel::moveRight));
        moveLButton->signal_clicked().connect(sigc::mem_fun(*this, &FoldableToolPanel::moveLeft));
  
        fudlrBox->pack_end( *trashButton,  Gtk::PACK_SHRINK, true, 0);

        fudlrBox->pack_end( *moveRButton,       Gtk::PACK_SHRINK, true, 0);
        fudlrBox->pack_end( *moveLButton,       Gtk::PACK_SHRINK, true, 0);
        fudlrBox->pack_end( *moveUButton,       Gtk::PACK_SHRINK, true, 0);
        fudlrBox->pack_end( *moveDButton,       Gtk::PACK_SHRINK, true, 0);
        fudlrBox->pack_end( *favoriteButton,  Gtk::PACK_SHRINK, true, 0);


//  exp->set_border_width (5);
//  exp->set_use_markup (true);
    if (need11) {
        Gtk::HBox *titleHBox = Gtk::manage(new Gtk::HBox());

        Gtk::Label *label = Gtk::manage(new Gtk::Label());
        label->set_markup(Glib::ustring("<b>") + escapeHtmlChars(UILabel) + Glib::ustring("</b>"));
        label->set_alignment(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER);
        titleHBox->pack_start(*label, Gtk::PACK_EXPAND_WIDGET, 0);

        RTImage *image = Gtk::manage (new RTImage("zoom-100-identifier.png"));
        image->set_tooltip_text(M("TP_GENERAL_11SCALE_TOOLTIP"));
        titleHBox->pack_end(*image, Gtk::PACK_SHRINK, 0);

        exp = Gtk::manage (new MyExpander (useEnabled, titleHBox, this ));
    } else {
        exp = Gtk::manage (new MyExpander (useEnabled, UILabel, this));
    }

        labelBox = exp->getHeaderHBox();
        labelBox->pack_start(*labelInfo, false, false,0);
        labelBox->pack_start(*fudlrBox, Gtk::PACK_EXPAND_WIDGET, true, 0);
//todo: this might not be necessary anymore
/*        labelWidget = Gtk::manage (new Gtk::Label("this will be updated later"));
        labelWidget->set_use_markup();
        labelBox->pack_start(*labelWidget, false, false, 0);
*/


    exp->signal_button_release_event().connect_notify( sigc::mem_fun(this, &FoldableToolPanel::foldThemAll) );
    enaConn = signal_enabled_toggled().connect( sigc::mem_fun(*this, &FoldableToolPanel::enabled_toggled) );

    exp->add (*content);
    exp->show ();
}

void FoldableToolPanel::foldThemAll (GdkEventButton* event)
{
    if (event->button == 3) {
        if (listener) {
            (static_cast<ToolPanelCoordinator*>(listener))->foldAllButOne( parentContainer, this);
        } else {
            (static_cast<ToolPanelCoordinator*>(tmp))->foldAllButOne( parentContainer, this);
        }
    }
}

void FoldableToolPanel::enabled_toggled()
{
    if (multiImage) {
        if (exp->get_inconsistent()) {
            exp->set_inconsistent (false);
            enaConn.block (true);
            exp->setEnabled (false);
            enaConn.block (false);
        } else if (lastEnabled) {
            exp->set_inconsistent (true);
        }

        lastEnabled = exp->getEnabled();
    }

    enabledChanged();
}

bool FoldableToolPanel::get_inconsistent()
{
    return exp->get_inconsistent();
}

void FoldableToolPanel::set_inconsistent(bool isInconsistent)
{
    exp->set_inconsistent(isInconsistent);
}

bool FoldableToolPanel::getEnabled()
{
    return exp->getEnabled();
}

// do not emit the enabled_toggled event
void FoldableToolPanel::setEnabled(bool isEnabled)
{
    enaConn.block (true);
    exp->setEnabled(isEnabled);
    lastEnabled = isEnabled;
    enaConn.block (false);
}

void FoldableToolPanel::setEnabledTooltipMarkup(Glib::ustring tooltipMarkup)
{
    if (exp) {
        exp->set_tooltip_markup(tooltipMarkup);
    }
}

void FoldableToolPanel::setEnabledTooltipText(Glib::ustring tooltipText)
{
    if (exp) {
        exp->set_tooltip_text(tooltipText);
    }
}

void FoldableToolPanel::deploy() 
{
// the empty implementation has to stay even if the method is empty because of linking issue if the implementation is in the .h include.

}
void FoldableToolPanel::deployLate()
{
}

DummyToolPanel::DummyToolPanel(Glib::ustring name, Environment* _env) : FoldableToolPanel(this,name,name,false,true)
{
  setToolName(name);
  env = _env;
}


