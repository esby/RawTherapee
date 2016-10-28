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

ToolVBoxDef::ToolVBoxDef()
{

   box = static_cast<ToolVBox*> (this);
   prevBox = nullptr;
   nextBox = nullptr;
   boxName = "undefined";
}

int ToolVBoxDef::size() {
             panelList = box->get_children ();
             return panelList.size ();
}

int ToolVBoxDef::getPos(ToolPanel* panel) {

  panelList = box->get_children ();
  Gtk::Widget *w = panel->getExpander();
  auto it = std::find(panelList.begin(), panelList.end(), w);
  if (it == panelList.end())
    return -1;
  else
    return  std::distance(panelList.begin(), it);
}

ToolPanel* ToolVBoxDef::getPanel(int pos) {
 panelList = box->get_children ();
 ToolPanel* p = static_cast<MyExpander*>(panelList[pos])->getPanel();
   return p;
}

// we initiate the nextBox too sinec it's circular
void ToolVBoxDef::setPrevBox(Gtk::VBox* _box) {
  prevBox = _box;
}


void ToolVBoxDef::setNextBox(Gtk::VBox* _box) {
  nextBox = _box;
}

Gtk::VBox* ToolVBoxDef::getPrevBox() {
  return prevBox;
}

Gtk::VBox* ToolVBoxDef::getNextBox() {
  return nextBox;
}


void ToolVBoxDef::remPanel(ToolPanel* t)
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


void ToolVBoxDef::addPanel(ToolPanel* t, int pos)
{
  ToolVBox* v = (ToolVBox*) this;
  MyExpander* exp = t->getExpander();
  int i = getPos(t);
  if (i==-1)
  {
//    printf("adding Panel: %s to %s \n", t->getToolName().c_str(), this->getBoxName().c_str());
    v->pack_start(*exp, false,false);
    if (pos !=-1)
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
  location = -1;
  favoriteBox->remPanel(favoriteDummy);
  originalBox->remPanel(originalDummy);
  favoriteBox->remPanel(this);
  originalBox->remPanel(this);
  trashBox->remPanel(this);
}


void ToolPanel::moveToFavorite(int posFav, int posOri)
{
  if (location != 0)
  {
    cleanBox();
    originalBox->addPanel(originalDummy,posOri);
    favoriteBox->addPanel(this, posFav);
    location = 0;
  }
}

void ToolPanel::moveToOriginal(int posFav, int posOri)
{
  if (location != 1)
  {
    cleanBox();
    if (posFav > -1)
      favoriteBox->addPanel(favoriteDummy,posFav);
    originalBox->addPanel(this, posOri);
    location = 1;
  }
}

void ToolPanel::moveToTrash(int posFav, int posOri)
{
 if (location != 2)
  {
    cleanBox();
    if (posFav>-1)
      favoriteBox->addPanel(favoriteDummy,posFav);
    originalBox->addPanel(originalDummy,posOri);
    trashBox->addPanel(this, -1); // there is no pos saved for this one
    location =2;
  }
}

int  ToolPanel::getPosOri()
{
  if (originalBox == nullptr)
    return -1;
  int posOri = originalBox->getPos(this);
  if (posOri == -1) posOri = originalBox->getPos(originalDummy);
  if (posOri == -1) posOri = originalBox->size()-2;
  return posOri;
}

int  ToolPanel::getPosFav()
{
  if (favoriteBox == nullptr)
    return -1;
  int posFav = favoriteBox->getPos(this);
  if (posFav == -1) posFav = favoriteBox->getPos(favoriteDummy);
  return posFav;
}

int  ToolPanel::getPosTra()
{
  if (trashBox == nullptr) 
    return -1;
  int posTra = trashBox->getPos(this);
  return posTra;
}


void ToolPanel::favorite_others_tabs_switch(int dc) 
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
    int posTra = getPosTra();
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


//     printf("name=%s  ", getToolName().c_str());
//     printf("positionOriginal=%i\n", originalBox->getPos(this));
     updateLabelInfo();

}

FoldableToolPanel::FoldableToolPanel(Gtk::Box* content, Glib::ustring toolName, Glib::ustring UILabel, bool need11, bool useEnabled) : ToolPanel(toolName, need11), parentContainer(nullptr), exp(nullptr), lastEnabled(true)

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
        labelBox->pack_end(*labelInfo, false, false,0);

        buttonBox = exp->getButtonHBox();
        buttonBox->pack_start(*fudlrBox, Gtk::PACK_EXPAND_WIDGET, true, 0);
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
// same remark as deploy()
}

void FoldableToolPanel::react(rtengine::ProcEvent ev)
{
// same remark as deploy()
}

DummyToolPanel::DummyToolPanel(Glib::ustring name, Environment* _env) : FoldableToolPanel(this,name,name,false,true)
{
  setToolName(name);
  env = _env;
}


