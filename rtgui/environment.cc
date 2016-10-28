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
ToolPanel* Environment::getPanel(Glib::ustring name)
{
  printf("call to a potentially slow method : getPanel of Environement\n");
  for (size_t i=0; i<toolPanels.size(); i++)
   {
      FoldableToolPanel* p = static_cast<FoldableToolPanel*> (toolPanels[i]);
      if (p != nullptr)
        if (p->getToolName() == name)
          return (ToolPanel*)p;
   }
  return nullptr;
}

ToolPanel* Environment::getPanel(int pos)
{
  if ((pos > -1)
  && (pos < toolPanels.size()))
  {
    FoldableToolPanel* p = static_cast<FoldableToolPanel*> (toolPanels[pos]);
    return p;
  }
  return nullptr;
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


