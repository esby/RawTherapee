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
#ifndef __MOVABLETOOLPANEL__
#define __MOVABLETOOLPANEL__

#include <gtkmm.h>
#include <glibmm.h>
#include "../rtengine/rtengine.h"
#include "../rtengine/procparams.h"
#include "multilangmgr.h"
#include "paramsedited.h"
//#include "edit.h"
#include "toolvboxdef.h"
#include "rtdef.h"
#include "guiutils.h"
#include "environment.h"

#define ENV_STATE_IN_FAV    1
#define ENV_STATE_IN_NORM   2
#define ENV_STATE_IN_TRASH  3



class MovableToolPanel
{
    protected:
    // 0 for favorite tabs.
    // 1 for normal tabs.
    // 2 for trash tabs.
    // this integer should be used to speed-up moveTo() methods that are not necessary.
    Environment* env;
    int location;
    Glib::ustring toolName;
    bool reacted;

    ToolVBox* originalBox;
    ToolVBox* favoriteBox;
    ToolVBox* trashBox;
    int fPosFav;
    int fPosOri;

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

    public:

   MovableToolPanel (Glib::ustring _toolName = "");

    virtual ~MovableToolPanel() {}

    virtual MyExpander* getExpander() // it will be redefined later
    {
      return nullptr;  
    }


    virtual bool                 canBeIgnored()      { return true; } // useful for determining if the panel is skippable or not.
    int                  getPosOri();
    int                  getPosFav();
    int                  getPosTra();
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
//    ToolPanel*      getFavoriteDummy() { return favoriteDummy; }
//    ToolPanel*      getOriginalDummy() { return originalDummy; }
//    ToolVBoxDef*         getOriginalBox() { return originalBox; }
    ToolVBox*         getOriginalBox() { return originalBox; }
    ToolVBox*            getFavoriteBox() { return favoriteBox;}


    virtual void cleanBox();
 // tt filters should reimplement these methods, normal filters should not need these
    virtual void themeImport(std::ifstream& myfile) {}
    virtual Glib::ustring themeExport() { return ""; }

   
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

    void                setToolName(Glib::ustring _name) { toolName = _name; }
    Glib::ustring               getToolName() { return toolName; } 
    void setLevel (int level);

    virtual int getLocation() {return location;}
    void setLocation(int _location) { location = _location;}

    virtual void deploy();
    virtual void deployLate();
    virtual void react(FakeProcEvent ev);

    virtual void moveToFavorite(int posFav, int posOri);
    virtual void moveToTrash(int posFav, int posOri);
    virtual void moveToOriginal(int posFav, int posOri);

    bool getReacted();
    void setReacted(bool _reacted);

};

Glib::ustring IntToString(int iVal);

#endif
