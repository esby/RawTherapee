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
#ifndef __TOOLVBOXDEF__
#define __TOOLVBOXDEF__

#include <gtkmm.h>
#include <glibmm.h>
#include "../rtengine/rtengine.h"
#include "../rtengine/procparams.h"
#include "guiutils.h"
#include "multilangmgr.h"
#include "paramsedited.h"
//#include "edit.h"
#include "rtdef.h"
#include "environment.h"
#include "movabletoolpanel.h"



// Nomenclature note:
// a "box" is either a tab containing toolpanels or a tool(panel) containing other tool(panel)s.
// a panel is a tool(panel).

class ToolVBoxDef 
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
      ToolVBoxDef();
      virtual ~ToolVBoxDef() {} ;
  
      virtual void setEnvironment(Environment* _env) { envTC = _env; }
      
      virtual void setPrevBox(Gtk::VBox* _box);
      virtual void setNextBox(Gtk::VBox* _box);

      virtual void setParent (Gtk::Container* parent) { parentContainer = parent; }
      virtual Gtk::Container* getParent () { return parentContainer; }

      virtual void setParentSW (Gtk::Container* parent) { parentSWContainer = parent; }
      virtual Gtk::Container* getParentSW () { return parentSWContainer; }


      virtual void setBoxName(Glib::ustring _name) { boxName = _name; }
      virtual Glib::ustring getBoxName() { return boxName; }

      // return the number of element the box contains.
      virtual int size();

      virtual int getPos(MovableToolPanel* panel);
      virtual MovableToolPanel* getPanel(int pos);

      // return the box a panel should be moved to when using left arrow.
      virtual Gtk::VBox* getPrevBox();
      // return the box a panel should be moved to when using right arrow.
      virtual Gtk::VBox* getNextBox();
      virtual void swapPanels(int pos1, int pos2);

      virtual void remPanel(MovableToolPanel* t);
      virtual void addPanel(MovableToolPanel* t, int pos);

};


#endif
