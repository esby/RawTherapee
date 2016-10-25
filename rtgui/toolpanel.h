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
#ifndef __TOOLPANEL__
#define __TOOLPANEL__

#include <gtkmm.h>
#include <glibmm.h>
#include "../rtengine/rtengine.h"
#include "../rtengine/procparams.h"
#include "guiutils.h"
#include "multilangmgr.h"
#include "paramsedited.h"
#include "edit.h"

#define ENV_STATE_IN_FAV    1
#define ENV_STATE_IN_NORM   2
#define ENV_STATE_IN_TRASH  3


class ToolPanel;
class FoldableToolPanel;

// Nomenclature note:
// a "box" is either a tab containing toolpanels or a tool(panel) containing other tool(panel)s.
// a panel is a tool(panel).


class DummyToolPanel;
class ToolVBox;


// the Environment is a class containing several information.
// the list of toolPanels.
// the list of corresponding expanders.
// the list of toolVbox 
// other information

// there are several instances of Environement inside the application, usually one per editor window and one for the main application.

class Environment {
  protected:
     std::vector<ToolPanel*> toolPanels;
     std::vector<MyExpander*> expList;
     std::vector<ToolVBox*> boxList;
     ToolVBox* favoritePanel;
     ToolVBox* trashPanel;
     Gtk::Notebook* toolPanelNotebook;

  public:
     int state;
     int prevState;
     bool moveLeftToBottom;
     bool moveRightToTop;
     bool disableSwitchPageReaction;
     bool metadataState;

     Environment(){ 
       state = ENV_STATE_IN_FAV;
       prevState = ENV_STATE_IN_FAV;
       moveLeftToBottom = false;
       moveRightToTop = true;
       disableSwitchPageReaction = true;
       metadataState = true; // true if metadata tab is here
     }

     virtual ~Environment() {} ;

     ToolPanel*  getPanel(Glib::ustring name);
     ToolPanel*  getPanel(int pos);     
     std::vector<ToolPanel*> getPanels() { return toolPanels; }
     std::vector<ToolPanel*>::iterator panelBegin();
     std::vector<ToolPanel*>::iterator panelEnd();  
     void erasePanel(std::vector<ToolPanel*>::iterator it);
     void panelPushBack(ToolPanel* p);
     void setMetadataState(bool b) {metadataState = b;}
     bool getMetadataState() {return metadataState;}

     MyExpander* getExpander(int pos);
     int countExpander();
     
     int countPanel();
     void setFavoritePanel(ToolVBox* p);
     void setTrashPanel(ToolVBox* p);

     void setToolPanelNotebook(Gtk::Notebook* _toolPanelNotebook) { toolPanelNotebook = _toolPanelNotebook;}
     Gtk::Notebook* getToolPanelNotebook() {return toolPanelNotebook; }

     void registerPanel(Gtk::Box* where, ToolPanel* panel);
     void addVBox(ToolVBox* box);
     std::vector<ToolVBox*> getVBoxList() { return boxList; }
    
     void reAttachPanel(ToolPanel *panel, ToolVBox* box, int pos);
     void setFavoritePos(ToolPanel *panel, int pos);

     ToolVBox* getFavoritePanel() { return favoritePanel; }
     ToolVBox* getTrashPanel() { return trashPanel; }

};


class ToolPanelListener
{

public:

    virtual ~ToolPanelListener() {}
    virtual void panelChanged   (rtengine::ProcEvent event, const Glib::ustring& descr) {}
};

class ToolCounter 
{
   protected:
     Environment* envTC;
     // this is a reference to the object itself, because i am lazy -- esby
     Gtk::VBox* box; 
     std::vector<Gtk::Widget*> panelList;

     Glib::ustring boxName;
     Gtk::Container* parentContainer;
     Gtk::Container* parentSWContainer;
 
     // references to other boxes
     Gtk::VBox* prevBox;
     Gtk::VBox* nextBox;

   public:
      ToolCounter();
      virtual ~ToolCounter() {} ;
  
      virtual void setEnvironment(Environment* _env) { envTC = _env; }
      
      virtual void setPrevBox(Gtk::VBox* _box);
      virtual void setNextBox(Gtk::VBox* _box);

      virtual void setParent (Gtk::Container* parent) { parentContainer = parent; }
      virtual Gtk::Container* getParent () { return parentContainer; }

      virtual void setParentSW (Gtk::Container* parent) { parentSWContainer = parent; }
      virtual Gtk::Container* getParentSW () { return parentSWContainer; }


      virtual void setBoxName(Glib::ustring _name) { boxName = _name; }
      virtual Glib::ustring getBoxName() { return boxName; }

      virtual int size();

      virtual int getPos(ToolPanel* panel);
      virtual ToolPanel* getPanel(int pos);

      virtual Gtk::VBox* getPrevBox();
      virtual Gtk::VBox* getNextBox();

      virtual void remPanel(ToolPanel* t);
      virtual void addPanel(ToolPanel* t, int pos);

};

/// @brief This class control the space around the group of tools inside a tab, as well as the space separating each tool. */
class ToolVBox : public Gtk::VBox, public ToolCounter 
{
private:
    void updateStyle();

public:
    ToolVBox();
    void on_style_changed (const Glib::RefPtr<Gtk::Style>& style);
};

/// @brief This class control the space around a tool's block of parameter. */
class ToolParamBlock : public Gtk::VBox, public ToolCounter 
{
private:
    void updateStyle();

public:
    ToolParamBlock();
    void on_style_changed (const Glib::RefPtr<Gtk::Style>& style);
};

class ToolPanel : public ToolPanelAncestor
{

protected:
    Glib::ustring toolName;
    ToolPanelListener* listener;
    ToolPanelListener* tmp;
    Environment* env;
    bool batchMode;  // True if the ToolPanel is used in Batch mode
    bool multiImage; // True if more than one image are being edited at the same time (also imply that batchMode=true), false otherwise
    bool need100Percent;
    // 0 for favorite tabs.
    // 1 for normal tabs.
    // 2 for trash tabs.
    // this integer should be used to speed-up moveTo() methods that are not necessary.
    int location;

    ToolVBox* originalBox;
    ToolVBox* favoriteBox;
    ToolVBox* trashBox;
    DummyToolPanel* originalDummy;
    DummyToolPanel* favoriteDummy;

    Glib::ustring uilabel;

    Gtk::Label* labelWidget;
    Gtk::Label* labelInfo;  
    Gtk::Button* labelInfoNotifier;
    Gtk::HBox* fudlrBox; // Favorite Up Down Left Right 

    Gtk::ToggleButton* favoriteButton;
    Gtk::ToggleButton* trashButton;
    Gtk::HBox* labelBox;
    Gtk::HBox* buttonBox;

    Gtk::Button* moveUButton;
    Gtk::Button* moveDButton;
    Gtk::Button* moveLButton;
    Gtk::Button* moveRButton;


    virtual void moveToFavorite(int posFav, int posOri);
    virtual void moveToTrash(int posFav, int posOri);

public:

    virtual void moveToOriginal(int posFav, int posOri);

    ToolPanel (Glib::ustring toolName = "", bool need11 = false) : toolName(toolName), listener(NULL), tmp(NULL), batchMode(false), multiImage(false), need100Percent(need11) 
    { 
          labelBox = NULL;
          buttonBox = NULL;
          location = 1; // normal panel location
    }
    virtual ~ToolPanel() {}

    virtual void           setParent       (Gtk::Box* parent) {}
    virtual Gtk::Box*      getParent       ()
    {
        return nullptr;
    }
    virtual MyExpander*    getExpander     ()
    {
        return nullptr;
    }
    virtual void           setExpanded     (bool expanded) {}
    virtual bool           getExpanded     ()
    {
        return false;
    }
    void           setMultiImage   (bool m)
    {
        multiImage = m;
    }
    void           setListener     (ToolPanelListener* tpl)
    {
        listener = tpl;
    }
    virtual void           setEditProvider (EditDataProvider *provider) {}
    virtual void           read            (const rtengine::procparams::ProcParams* pp, const ParamsEdited* pedited = nullptr) {}
    virtual void           write           (rtengine::procparams::ProcParams* pp, ParamsEdited* pedited = nullptr) {}
    virtual void           trimValues      (rtengine::procparams::ProcParams* pp)
    {
        return;
    }
    virtual void           setDefaults     (const rtengine::procparams::ProcParams* defParams, const ParamsEdited* pedited = nullptr) {}
    virtual void           autoOpenCurve   () {}

    virtual bool                 canBeIgnored()      { return true; } // useful for determining if the panel is skippable or not.
    int                  getPosOri();
    int                  getPosFav();
    int                  getPosTra();
 //   virtual Gtk::CheckButton*    getEnabledButton()  { return enabledButtonRef;}
    virtual Gtk::ToggleButton*   getFavoriteButton() { return favoriteButton;}
    virtual Gtk::ToggleButton*   getTrashButton()    { return trashButton;}
    virtual Gtk::HBox*           getLabelBox()       { return labelBox;}
    virtual Gtk::HBox*           getButtonBox()       { return buttonBox;}
    virtual Gtk::Button*         getLabelInfoNotifier() { return labelInfoNotifier;}
    Gtk::HBox*           getFUDLRBox()  { return fudlrBox; }                
    Gtk::Button*         getMoveUButton() { return moveUButton; }
    Gtk::Button*         getMoveDButton() { return moveDButton; }
    Gtk::Button*         getMoveLButton() { return moveLButton; }
    Gtk::Button*         getMoveRButton() { return moveRButton; }
    void                 setOriginalBox(ToolVBox* tc) {originalBox = tc; }
    ToolCounter*         getOriginalBox() { return originalBox; }
    DummyToolPanel*      getFavoriteDummy() { return originalDummy;}
    ToolVBox*            getFavoriteBox() { return favoriteBox;}

    virtual void cleanBox();
 // tt filters should reimplement these methods, normal filters should not need these
    virtual void themeImport(std::ifstream& myfile) {}
    virtual Glib::ustring themeExport() { return ""; }

   
    virtual void                 deploy()       {} // used to handle post operations steps.
    virtual void        deployLate() {} // used to handle post operations in a later way than deploy ie: for hiding stuf.
    virtual void react(rtengine::ProcEvent ev) {} // used to react to external event like image loading / image saving / profile loading etc. 

    void                setToolName(Glib::ustring _name) { toolName = _name; }
    Glib::ustring               getToolName() { return toolName; } 
  
    void                setNeed100Percent(bool b) { need100Percent = b; }
    bool                getNeed100Percent() { return need100Percent; }

    virtual int getLocation() {return location;}
    int setLocation(int _location) { location = _location;}

    /** @brief Disable the event broadcasting mechanism
     *
     * @return Return the previous state of the broadcast (true: enabled ; false: disabled)
     */
    bool disableListener ()
    {
        if (tmp == nullptr) {
            tmp = listener;
        }

        bool prevState = listener != nullptr;
        listener = nullptr;
        return prevState;
    }

    /** @brief Enable the event broadcasting mechanism
     */
    void enableListener  ()
    {
        if (tmp != nullptr) {
            listener = tmp;
        }

        tmp = nullptr;
    }

    virtual void setBatchMode    (bool batchMode)
    {
        this->batchMode = batchMode;
    }

    Glib::ustring getThemeInfo ();
    void favorite_others_tabs_switch(int dc);
    void initVBox(ToolVBox* _originalBox, ToolVBox* _favoriteBox, ToolVBox* _trashBox, Environment* _env);
    void on_toggle_button_favorite();
    void on_toggle_button_trash();

    void moveUp(); 
    void moveDown();
    void moveLeft();
    void moveRight();
    void updateLabelInfo ();

};

class FoldableToolPanel : public ToolPanel
{

protected:
    Gtk::Box* parentContainer;
    MyExpander* exp;
    bool lastEnabled;
    sigc::connection enaConn;
    void foldThemAll (GdkEventButton* event);
    void enabled_toggled();

public:

    FoldableToolPanel(Gtk::Box* content, Glib::ustring toolName, Glib::ustring UILabel, bool need11 = false, bool useEnabled = false);

    MyExpander* getExpander()
    {
        return exp;
    }
    void setExpanded (bool expanded)
    {
        if (exp) {
            exp->set_expanded( expanded );
        }
    }
    bool getExpanded ()
    {
        if (exp) {
            return exp->get_expanded();
        }

        return false;
    }
    void setParent (Gtk::Box* parent)
    {
        parentContainer = parent;
    }
    Gtk::Box* getParent ()
    {
        return parentContainer;
    }

    virtual void enabledChanged  () {}

    bool getUseEnabled ()
    {
        if (exp) {
            return exp->getUseEnabled();
        } else {
            return true;
        }
    }
    bool getEnabled();  // related to the enabled/disabled state
    void setEnabled(bool isActive);  // related to the enabled/disabled state
    void setEnabledTooltipMarkup(Glib::ustring tooltipMarkup);
    void setEnabledTooltipText(Glib::ustring tooltipText);
    bool get_inconsistent();  // related to the enabled/disabled state
    void set_inconsistent(bool isInconsistent);  // related to the enabled/disabled state

    void deploy();
    void deployLate(); 
    void react(rtengine::ProcEvent ev);
    bool canBeIgnored() {return false;}

    // Functions that want to receive an enabled/disabled event from this class
    // will have to receive it from MyExpander directly, we do not create
    // a relaying event
    MyExpander::type_signal_enabled_toggled signal_enabled_toggled()
    {
        return exp->signal_enabled_toggled();
    }
};

// this class is used to keep the position of a favorited or trashed panel. this is used to preserve the orders of the toolpanel relative to the others in its Vbox.
class DummyToolPanel : public ToolParamBlock , public FoldableToolPanel {

  protected:


  public:

        DummyToolPanel(Glib::ustring, Environment* _env);
        bool   canBeIgnored() { return true; }
};

Glib::ustring IntToString(int iVal);


#endif
