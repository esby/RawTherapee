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
