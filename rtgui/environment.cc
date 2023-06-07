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
//#include "toolpanel.h"
#include "toolpanelcoord.h"
#include "guiutils.h"

using namespace rtengine::procparams;

int nbEnv = 0;

Environment::Environment(std::vector<ToolPanel*>& _toolPanels,  std::vector<MyExpander*>& _expList) 
: toolPanels( _toolPanels), expList( _expList)
{
  envId = nbEnv;
  nbEnv++;
  state = ENV_STATE_IN_FAV;
  prevState = ENV_STATE_IN_FAV;
  moveLeftToBottom = false; // moving left adds to top.
  moveRightToTop = true; // moving right adds to top.
  disableSwitchPageReaction = true;
  metadataState = true; // true if metadata tab is here
  customVariableCount = 0;
  profilepanel=nullptr;
  doLog=false;
}

Environment::~Environment()
{
  //todo: free rt variable
  
  printf("deleting rt variables due to the environment destruction \n");
  while (varList.size()>0)
  {
   delete (varList[0]);
   varList.erase(varList.begin());;
  }

}

// Environment implementation
MovableToolPanel* Environment::getPanel(Glib::ustring name)
{
  printf("call to a potentially slow method : getPanel of Environement\n");
  for (size_t i=0; i<toolPanels.size(); i++)
   {
      MovableToolPanel* p = static_cast<MovableToolPanel*> (toolPanels[i]);
      if (p != nullptr)
        if (p->getToolName() == name)
          return (ToolPanel*)p;
   }
  return nullptr;
}

ToolPanel* Environment::getPanel(int pos)
{
  if ((pos > -1)
  && (pos < (signed int)(toolPanels.size())))
  {
    FoldableToolPanel* p = static_cast<FoldableToolPanel*> (toolPanels[pos]);
    return p;
  }
  return nullptr;
}

/*
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

size_t Environment::countPanel()
{
  return toolPanels.size();
}

void  Environment::panelPushBack(ToolPanel* p)
{
  toolPanels.push_back(p);
}
*/

MyExpander* Environment::getExpander(int pos)
{
  return expList.at(pos);
}

size_t Environment::countExpander()
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

void Environment::registerPanel(Gtk::Box* where, ToolPanel* panel, int level)
{
  panel->setParent (where);
  panel->setLevel (level);
  expList.push_back (panel->getExpander());
  where->pack_start (*panel->getExpander(), false, false);
  panel->initVBox ((ToolVBox*)where, favoritePanel, trashPanel, this);
  toolPanels.push_back (panel);
}

void Environment::addVBox(ToolVBox* box) 
{ 
  boxList.push_back(box);
}

void Environment::reAttachPanel(ToolPanel *panel, ToolVBox* box, int pos)
{
  printf("reattaching panel %s from %s to %s \n", panel->getToolName().c_str(), panel->getOriginalBox()->getBoxName().c_str(), box->getBoxName().c_str());
//  Gtk::Container* c = (Gtk::Container*)box;
  panel->getOriginalBox()->remPanel(panel);
  box->addPanel(panel, pos);
}

void Environment::setFavoritePos(ToolPanel *panel, int pos)
{
  panel->getOriginalBox()->remPanel(panel);
//  printf("panel:%s box:%s \n", panel->getToolName().c_str(), panel->getOriginalBox()->getBoxName().c_str());
  if (pos > -1)
    panel->getFavoriteBox()->addPanel(panel, pos);
  panel->setPosFav(pos);
  panel->setPLocation(PANEL_ON_FAVORITE);
}

RtVariable* Environment::getVariable(int pos)
{
  if ((pos > -1) && (size_t)pos<countVar())
    return varList[pos] ;
  return nullptr;
}

Glib::ustring Environment::getVariableName(int pos)
{
  if ((pos > -1) && (size_t)pos<countVar())
    return  varList[pos]->getName();
  return "";
}

Glib::ustring Environment::invokeCustomVariable()
{
  Glib::ustring name = "var" + std::to_string(customVariableCount);
  customVariableCount++;
  setVar(name,name);
  return name;
}


void Environment::setVar(Glib::ustring name, Glib::ustring value)
{ 
  RtVariable* d = nullptr;
  RtVariable* e = nullptr;
  for (size_t i=0; i<varList.size(); i++)
   {
      e = static_cast<RtVariable*> (varList[i]);
      if (e != nullptr)
        if (e->getName() == name)
          d = e;
   }

   if (d == nullptr)
   {
     d = new RtVariable(name, this);
     varList.push_back(d);
   }
   d->setAsString(value);
}

void Environment::setVar(Glib::ustring name, int value)
{
  RtVariable* d = nullptr;
  RtVariable* e = nullptr;
  for (size_t i=0; i<varList.size(); i++)
   {
      e = static_cast<RtVariable*> (varList[i]);
      if (e != nullptr)
        if (e->getName() == name)
          d = e;
   }

   if (d == nullptr)
   {
     d = new RtVariable(name, this);
     varList.push_back(d);
   }
   d->setAsInt(value);
}

void Environment::setVar(Glib::ustring name, double value)
{
  RtVariable* d = nullptr;
  RtVariable* e = nullptr;
  for (size_t i=0; i<varList.size(); i++)
   {
      e = static_cast<RtVariable*> (varList[i]);
      if (e != nullptr)
        if (e->getName() == name)
          d = e;
   }

   if (d == nullptr)
   {
     d = new RtVariable(name, this);
     varList.push_back(d);
   }
   d->setAsDouble(value);
}

void Environment::setVar(Glib::ustring name, bool value)
{
  RtVariable* d = nullptr;
  RtVariable* e = nullptr;
  for (size_t i=0; i<varList.size(); i++)
   {
      e = static_cast<RtVariable*> (varList[i]);
      if (e != nullptr)
        if (e->getName() == name)
          d = e;
   }

   if (d == nullptr)
   {
     d = new RtVariable(name, this);
     varList.push_back(d);
   }
   d->setAsBool(value);
}

RtVariable* Environment::getVariableByName(Glib::ustring name)
{
  for (size_t i=0; i<varList.size(); i++)
   {
      RtVariable* d = static_cast<RtVariable*> (varList[i]);
      if (d != nullptr)
        if (d->getName() == name)
          return d;
   }
  return nullptr;
}

int Environment::getVarAsInt(Glib::ustring name)
{
  for (size_t i=0; i<varList.size(); i++)
   {
      RtVariable* d = static_cast<RtVariable*> (varList[i]);
      if (d != nullptr)
        if (d->getName() == name)
          return d->getAsInt();
   }
  return -1;
}

double Environment::getVarAsDouble(Glib::ustring name)
{
  for (size_t i=0; i<varList.size(); i++)
   {
      RtVariable* d = static_cast<RtVariable*> (varList[i]);
      if (d != nullptr)
        if (d->getName() == name)
          return d->getAsDouble();
   }
  return -1;
}

bool Environment::getVarAsBool(Glib::ustring name)
{
  for (size_t i=0; i<varList.size(); i++)
   {
      RtVariable* d = static_cast<RtVariable*> (varList[i]);
      if (d != nullptr)
        if (d->getName() == name)
          return d->getAsBool();
   }
  return false;
}

Glib::ustring Environment::getVarAsString(Glib::ustring name)
{
  for (size_t i=0; i<varList.size(); i++)
   {
      RtVariable* d = static_cast<RtVariable*> (varList[i]);
      if (d != nullptr)
        if (d->getName() == name)
          return d->getAsString();
   }
  return "";
}

Glib::ustring Environment::getExifVariable(Glib::ustring name)
{
  Glib::ustring pname = ROOT_EXIF_PREFIX + ':' + name;
  Glib::ustring sval =  getVarAsString(pname);
  printf("%s= %s \n", pname.c_str(), sval.c_str());
  return sval;
}


void Environment::setProfilePanel(ProfilePanel* _profilep)
{
  profilepanel = _profilep;
}

ProfilePanel*  Environment::getProfilePanel()
{
  return profilepanel;
}

int Environment::getEnvRef()
{
  return envId;

}

