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
#include "movabletoolpanel.h"
#include "toolpanelcoord.h"


using namespace rtengine::procparams;

MovableToolPanel::MovableToolPanel (Glib::ustring _toolName )  
{
    setToolName(_toolName);
    location = 1; // normal panel location

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
    favoriteButton->signal_toggled().connect( sigc::mem_fun(*this, &MovableToolPanel::on_toggle_button_favorite) );
    trashButton->signal_toggled().connect( sigc::mem_fun(*this, &MovableToolPanel::on_toggle_button_trash) );

    moveUButton->signal_clicked().connect(sigc::mem_fun(*this, &MovableToolPanel::moveUp));
    moveDButton->signal_clicked().connect(sigc::mem_fun(*this, &MovableToolPanel::moveDown));
    moveRButton->signal_clicked().connect(sigc::mem_fun(*this, &MovableToolPanel::moveRight));
    moveLButton->signal_clicked().connect(sigc::mem_fun(*this, &MovableToolPanel::moveLeft));

    fudlrBox->pack_end( *trashButton,  Gtk::PACK_SHRINK, true, 0);

    fudlrBox->pack_end( *moveRButton,       Gtk::PACK_SHRINK, true, 0);
    fudlrBox->pack_end( *moveLButton,       Gtk::PACK_SHRINK, true, 0);
    fudlrBox->pack_end( *moveUButton,       Gtk::PACK_SHRINK, true, 0);
    fudlrBox->pack_end( *moveDButton,       Gtk::PACK_SHRINK, true, 0);
    fudlrBox->pack_end( *favoriteButton,  Gtk::PACK_SHRINK, true, 0);
            
}

void MovableToolPanel::initVBox(ToolVBox* _originalBox, ToolVBox* _favoriteBox, ToolVBox* _trashBox, Environment* _env){
//     printf("initVBox for name=%s \n", getToolName().c_str());

     env = _env;   
     buttonBox = getExpander()->getButtonHBox();
     buttonBox->pack_start(*fudlrBox); //, Gtk::PACK_EXPAND_WIDGET, true, 0); 

     originalBox = _originalBox;
     favoriteBox = _favoriteBox;
     trashBox = _trashBox;

//     printf("name=%s  ", getToolName().c_str());
//     printf("positionOriginal=%i\n", originalBox->getPos(this));
     updateLabelInfo();

     reacted = false;

}

void MovableToolPanel::updateLabelInfo() {
 if ((this->getExpander() != nullptr)
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
    if (enabledButtonRef!=nullptr){
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

void MovableToolPanel::moveUp () {
  int pos;
  int npos;
 // int count;
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
// count = box->size();

  if (npos > -1){
//     printf("- realized.\n");
     box->reorder_child(*getExpander(), npos); 
     updateLabelInfo();
     box->getPanel(pos)->updateLabelInfo(); // since we swapped it's pos and not npos
  }//else  printf("- canceled.\n");
}

void MovableToolPanel::moveDown () { 
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

void MovableToolPanel::moveLeft() {
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
    Glib::RefPtr<Gtk::Adjustment> adj =  parentSw->get_vadjustment();
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

void MovableToolPanel::moveRight() {
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
    Glib::RefPtr<Gtk::Adjustment> adj = parentSw->get_vadjustment();
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

void MovableToolPanel::cleanBox() {
  location = -1;
  favoriteBox->remPanel(this);
  originalBox->remPanel(this);
  trashBox->remPanel(this);
}

void MovableToolPanel::moveToFavorite(int posFav, int posOri)
{
  printf("location %i\n",location);
  if (location != 0)
  {
    cleanBox();
    fPosOri = posOri;
    favoriteBox->addPanel(this, posFav);
    location = 0;
  }
}

void MovableToolPanel::moveToOriginal(int posFav, int posOri)
{
  if (location != 1)
  {
    cleanBox();
    if (posFav > -1)
       fPosFav = posFav;
    originalBox->addPanel(this, posOri);
    location = 1;
  }
}

void MovableToolPanel::moveToTrash(int posFav, int posOri)
{
 if (location != 2)
  {
    cleanBox();
    if (posFav>-1)
      fPosFav = posFav;
    fPosOri = posOri;      
    trashBox->addPanel(this, -1); // there is no pos saved for this one
    location =2;
  }
}

int  MovableToolPanel::getPosOri()
{
  if (originalBox == nullptr)
    return -1;
  int posOri = originalBox->getPos(this);
  if (posOri == -1) posOri = fPosOri;
  if (posOri == -1) posOri = originalBox->size()-2;
  return posOri;
}

int  MovableToolPanel::getPosFav()
{
  if (favoriteBox == nullptr)
    return -1;
  int posFav = favoriteBox->getPos(this);
  if (posFav == -1) posFav = fPosFav;
  return posFav;
}

int  MovableToolPanel::getPosTra()
{
  if (trashBox == nullptr) 
    return -1;
  int posTra = trashBox->getPos(this);
  return posTra;
}


void MovableToolPanel::favorite_others_tabs_switch(int dc) 
{

  if ((this->getExpander() != nullptr)
    && ((!this->canBeIgnored())))
  {

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

    int posOri = getPosOri();
    int posFav = getPosFav();
    //int posTra = getPosTra();
    //    printf("dc=%i\n",dc);

    // handling all the possible cases
    switch(dc)
    {
      case 11:                   // fav to fav
        //we do nothing
        break;
      case 12:                   // fav to normal
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
        break;
      case 13:                   // fav to trash
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
        break;
      case 21:                   // normal to favorite
        if (favoriteButton->get_active())
        {
          moveToFavorite(posFav,posOri);
        }
        else
        {
          moveToOriginal(posFav, posOri);
        }
        break;

      case 22:                   // normal to normal
        if (trashButton->get_active())
        {
          moveToTrash(posFav,posOri);
        }
        else
        {
          moveToOriginal(posFav,posOri);
        }
        break;

      case 23:                   // normal -> trash
        if (trashButton->get_active())
        {
          moveToTrash(posFav,posOri);
        }
        else                     // resume check here
        {
          moveToOriginal(posFav, posOri);
        }
        break;

      case 31:                   // trash -> favorite
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
        else                     // not trash
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
        break;

      case 32:                   // trash -> normal
        if (!trashButton->get_active())
        {
          moveToOriginal(posFav,posOri);
        }
        break;

      case 33:                   // trash -> trash - nothing to be done
        break;
    }
  }
}

void MovableToolPanel::on_toggle_button_favorite() 
{

}

void MovableToolPanel::on_toggle_button_trash() {

}


Glib::ustring MovableToolPanel::getThemeInfo() {
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

void MovableToolPanel::setLevel (int level)
{
    if (getExpander()) {
        getExpander()->setLevel(level);
    }
}

void MovableToolPanel::deploy() 
{
// the empty implementation has to stay even if the method is empty because of linking issue if the implementation is in the .h include.

}

void MovableToolPanel::deployLate()
{
// same remark as deploy()
}

void MovableToolPanel::react(FakeProcEvent ev)
{
// same remark as deploy()
}


Glib::ustring IntToString(int iVal)
{
    std::ostringstream ssIn;
    ssIn << iVal;
    Glib::ustring strOut = ssIn.str();
    return strOut;
}

bool MovableToolPanel::getReacted()
{
  return reacted;
}

void MovableToolPanel::setReacted(bool _reacted)
{
  reacted = _reacted;
}

