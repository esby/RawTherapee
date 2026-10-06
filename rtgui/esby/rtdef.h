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
#ifndef __RTDEF__
#define __RTDEF__


#define  PANEL_NAME_FAVORITE  "favoritePanel"
#define  PANEL_NAME_EXPOSURE  "exposurePanel"
#define  PANEL_NAME_DETAILS   "detailsPanel"
#define  PANEL_NAME_COLOR     "colorPanel" 
#define  PANEL_NAME_WAVELET   "waveletPanel"
#define PANEL_NAME_LOCALLAB   "locallabPanel"
#define  PANEL_NAME_TRANSFORM "transformPanel"
#define  PANEL_NAME_RAW       "rawPanel" 
// it lacks metadata definition, but it's normal.
#define  PANEL_NAME_USEFUL    "usefulPanel"
#define  PANEL_NAME_TRASH     "trashPanel"

const Glib::ustring ROOT_EXIF_PREFIX="rti";

enum FakeProcEvent {
    FakeEvExifTransmitted = 1,
    FakeEvPhotoLoaded= 2,
    FakeEvProfileChanged=3,
    FakeEvFileSaved=4,
    FakeEvPP3Transmitted=5,
    FakeEvShowAllTriggered=6,
    FakeEvFullExifTransmitted=7,
    nbFakeEvents
    }; 

class Environment;
class MovableToolPanel;
class ToolPanel;
class FoldableToolPanel;
class ToolVBox;
class RtVariable;
class RtVariableOperator;
class RtVariableManipulator;


#endif
