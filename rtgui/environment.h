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
#ifndef __RTENVIRONMENT__
#define __RTENVIRONMENT__

#include <gtkmm.h>
#include <glibmm.h>
#include "../rtengine/rtengine.h"
#include "../rtengine/procparams.h"
#include "rtdef.h"
#include "guiutils.h"
#include "movabletoolpanel.h"
//include "toolplanel.h"
#include "variable.h"
#include "profilepanel.h"

#define ENV_STATE_IN_FAV    1
#define ENV_STATE_IN_NORM   2
#define ENV_STATE_IN_TRASH  3

// Nomenclature note:
// a "box" is either a tab containing toolpanels or a tool(panel) containing other tool(panel)s.
// a panel is a tool(panel).




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
     std::vector<RtVariable*> varList;
     int customVariableCount; // for creating unique custom variable
     ProfilePanel* profilepanel ;

  public:
     int envId;
     int state;
     int prevState;
     // this can be customized in theory - panel can be moved to bottom instead of top when moving left.
     bool moveLeftToBottom;
     bool moveRightToTop;
     bool disableSwitchPageReaction;
     bool metadataState;

     Environment(); 

     virtual ~Environment() ;

     MovableToolPanel*  getPanel(Glib::ustring name);
     ToolPanel*  getPanel(int pos);     
     void setToolPanels(std::vector<ToolPanel*> _toolPanels) { toolPanels = _toolPanels; }
     std::vector<ToolPanel*> getToolPanels() { return toolPanels; }
//     std::vector<ToolPanel*>::iterator panelBegin();
//     std::vector<ToolPanel*>::iterator panelEnd();  
//     void erasePanel(std::vector<ToolPanel*>::iterator it);
//     void panelPushBack(ToolPanel* p);
     void setMetadataState(bool b) {metadataState = b;}
     bool getMetadataState() {return metadataState;}
     int getId() { return envId; }

     MyExpander* getExpander(int pos);
     size_t countExpander();
     
     size_t countPanel();
     void setFavoritePanel(ToolVBox* p);
     void setTrashPanel(ToolVBox* p);

     void setToolPanelNotebook(Gtk::Notebook* _toolPanelNotebook) { toolPanelNotebook = _toolPanelNotebook;}
     Gtk::Notebook* getToolPanelNotebook() {return toolPanelNotebook; }

     void registerPanel(Gtk::Box* where, ToolPanel* panel, int level);
     void addVBox(ToolVBox* box);
     std::vector<ToolVBox*> getVBoxList() { return boxList; }
    
     void reAttachPanel(ToolPanel *panel, ToolVBox* box, int pos);
     void setFavoritePos(ToolPanel *panel, int pos);

     ToolVBox* getFavoritePanel() { return favoritePanel; }
     ToolVBox* getTrashPanel() { return trashPanel; }

     //variables related code follow

     size_t countVar(){return varList.size();}
     RtVariable* getVariable(int pos);
     RtVariable* getVariableByName(Glib::ustring name);

     Glib::ustring getVariableName(int pos);
 
     // create a custom variable and return its name
     Glib::ustring invokeCustomVariable();

     void setVar(Glib::ustring name, Glib::ustring value);
     void setVar(Glib::ustring name, int value);
     void setVar(Glib::ustring name, double value);
     void setVar(Glib::ustring name, bool value);

     Glib::ustring getVarAsString(Glib::ustring name);
     int getVarAsInt(Glib::ustring name);
     double getVarAsDouble(Glib::ustring name);
     bool getVarAsBool(Glib::ustring name);

     Glib::ustring getExifVariable(Glib::ustring name);

     void setProfilePanel(ProfilePanel* _profilep);
     ProfilePanel* getProfilePanel();

     int getEnvRef();
  
};

#endif
