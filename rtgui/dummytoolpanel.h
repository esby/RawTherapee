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
#ifndef __MOVABLRPANEL__
#define __MOVABLELPANEL__

#include <gtkmm.h>
#include <glibmm.h>
#include "../rtengine/rtengine.h"
#include "../rtengine/procparams.h"
#include "multilangmgr.h"
#include "paramsedited.h"
#include "edit.h"
#include "toolvboxdef.h"
#include "rtdef.h"
#include "guiutils.h"
#include "environment.h"


// this class is used to keep the position of a favorited or trashed panel. this is used to preserve the orders of the toolpanel relative to the others in its Vbox.
class DummyToolPanel : public ToolParamBlock , public FoldableToolPanel {

  protected:


  public:

        DummyToolPanel(Glib::ustring, Environment* _env);
        bool   canBeIgnored() { return true; }
};


#endif
