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
#include <algorithm>
#include <climits>
#include <map>
#include <set>
#include <vector>


using namespace rtengine;
using namespace rtengine::procparams;


/*
bool sortByFavW(Gtk::Widget* t1, Gtk::Widget* t2)
{
 ToolVBox* p1 = static_cast<ToolVBox*>(t1);
  ToolVBox* p2 = static_cast<ToolVBox*>(t2);

 return sortByFav(p1,p2);
}

bool sortByOriW(Gtk::Widget* t1, Gtk::Widget* t2)
{
  ToolVBox* p1 = static_cast<ToolVBox*>(t1);
  ToolVBox* p2 = static_cast<ToolVBox*>(t2);

  return sortByOri(p1,p2);
}
*/


// note: std::sort requires a strict weak ordering.
// ignorable panels are equivalent between themselves and sorted after the others.
bool sortByFav(ToolPanel* t1, ToolPanel* t2)
{
  const bool i1 = t1->canBeIgnored();
  const bool i2 = t2->canBeIgnored();
  if (i1 || i2) return (!i1 && i2);

  return (t1->getPosFav() < t2->getPosFav());
}


bool sortByOri(ToolPanel* t1, ToolPanel* t2)
{
  const bool i1 = t1->canBeIgnored();
  const bool i2 = t2->canBeIgnored();
  if (i1 || i2) return (!i1 && i2);
  
  
  if (t1->getOriginalBox()->getBoxName() == t2->getOriginalBox()->getBoxName())
    return (t1->getPosOri() < t2->getPosOri());
  else return (t1->getOriginalBox()->getBoxName() < t2->getOriginalBox()->getBoxName());
}

// position of the panels in their original box, as if every panel was in it.
// when the favorite tab is displayed, the favorite panels are not in their original box and
// the other panels are shifted: the panels present in the box are taken in their current
// order, and the absent ones are inserted back at their last known position (getPosOri()).
// in a normal tab, the result is the position of the panel in its box (getPos()).
// includeTrash: the trash panels are part of the order (their place is kept when they are out of
// their box, in the trash tab); otherwise they are ignored (ex: ttp export, their position is not saved).
std::map<ToolPanel*, int> computeOriginalPositions(const std::vector<ToolPanel*>& panels, bool includeTrash)
{
  std::map<ToolVBox*, std::vector<std::pair<int, ToolPanel*>>> present;
  std::map<ToolVBox*, std::vector<ToolPanel*>> absent;

  for (auto p : panels)
  {
    FoldableToolPanel* fp = static_cast<FoldableToolPanel*>(p);
    if ((fp == nullptr) || fp->canBeIgnored())
      continue;
    if (!includeTrash && fp->getTrashButton()->get_active())
      continue;
    ToolVBox* box = fp->getOriginalBox();
    if (box == nullptr)
      continue;
    int pos = box->getPos(fp);
    if (pos >= 0)
      present[box].push_back(std::make_pair(pos, p));
    else
      absent[box].push_back(p);
  }

  std::map<ToolPanel*, int> result;
  std::set<ToolVBox*> boxes;
  for (auto& b : present) boxes.insert(b.first);
  for (auto& b : absent) boxes.insert(b.first);

  for (auto box : boxes)
  {
    std::vector<std::pair<int, ToolPanel*>>& inBox = present[box];
    std::sort(inBox.begin(), inBox.end(),
              [](const std::pair<int, ToolPanel*>& a, const std::pair<int, ToolPanel*>& b) { return a.first < b.first; });
    std::vector<ToolPanel*> order;
    for (auto& e : inBox)
      order.push_back(e.second);

    // absent panels inserted back by increasing last known position, unknown positions at the end
    std::vector<ToolPanel*>& out = absent[box];
    std::stable_sort(out.begin(), out.end(), [](ToolPanel* a, ToolPanel* b) {
      int pa = a->getPosOri() < 0 ? INT_MAX : a->getPosOri();
      int pb = b->getPosOri() < 0 ? INT_MAX : b->getPosOri();
      return pa < pb;
    });
    for (auto p : out)
    {
      int pos = p->getPosOri();
      if ((pos < 0) || (pos > (int)order.size()))
        order.push_back(p);
      else
        order.insert(order.begin() + pos, p);
    }

    for (size_t i = 0; i < order.size(); i++)
      result[order[i]] = i;
  }
  return result;
}
