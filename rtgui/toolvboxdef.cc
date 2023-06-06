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
#include "toolvboxdef.h"
//#include "guiutils.h"

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

int ToolVBoxDef::getPos(MovableToolPanel* panel) {

  panelList = box->get_children ();
  Gtk::Widget *w = panel->getExpander();
  auto it = std::find(panelList.begin(), panelList.end(), w);
  if (it == panelList.end())
  {
//   if (panel->getToolName()=="tonecurve")
//     printf("pos=-1 found for tonecurve\n");
    return -1;
  }
  else
  {
    auto d = std::distance(panelList.begin(), it);
//    if (panel->getToolName()=="tonecurve")
//      printf("using std:distance=%lu\n",d);
    return d;
 }
}

MovableToolPanel* ToolVBoxDef::getPanel(int pos) {
 panelList = box->get_children ();
 MovableToolPanel* p = static_cast<MyExpander*>(panelList[pos])->getPanel();
   return p;
}

void ToolVBoxDef::swapPanels(int pos1, int pos2)
{
 MovableToolPanel* t = getPanel(pos1);
 MyExpander* exp = t->getExpander();

 panelList = box->get_children ();
 panelList[pos1] = panelList[pos2];
 panelList[pos2] = exp;

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


void ToolVBoxDef::remPanel(MovableToolPanel* t)
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

void ToolVBoxDef::addPanel(MovableToolPanel* t, int pos)
{
  ToolVBox* v = (ToolVBox*) this;
  MyExpander* exp = t->getExpander();
  int i = getPos(t);
  if (i==-1)
  {
//    printf("ToolVBoxDef adding Panel: %s to %s \n", t->getToolName().c_str(), this->getBoxName().c_str());
    v->pack_start(*exp, false,false);
    if (pos !=-1)
      v->reorder_child(*exp, pos);
  }
  else
   printf("ToolVBoxDef::addPanel getPos non negative encountered %i",i);
}

