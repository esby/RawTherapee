/*
 *  This file is part of RawTherapee.
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
#pragma once

#include <glibmm/keyfile.h>
#include <glibmm/ustring.h>

// extension of the ttp (tool layout) profiles
extern Glib::ustring paramFileGuiExtension;

// translation file of the esby fork (keys of the esby code), loaded after the upstream files
Glib::ustring esbyTranslationFile();

// options of the esby fork, stored in the [TTP] group of the options file.
// it is a member of Options (Options::esby): it follows the copies made by the
// preferences dialog (moptions), and only a few hooks are needed in options.h / options.cc.
// note: every member has a default value, the TTP options were not initialized before.
class EsbySettings
{
public:
    bool TTPAutoload = false;  // to load the ttp profiles ot not by default
    Glib::ustring TTPAutoloadValue; // the profile to load
    bool TTPHideFavorite = false;
    bool TTPHideExposure = false;
    bool TTPHideDetails = false;
    bool TTPHideColor = false;
    bool TTPHideAdvanced = false;
    bool TTPHideLocal = false;
    bool TTPHideTransform = false;
    bool TTPHideRaw = false;
    bool TTPHideMetadata = false;
    bool TTPHideUseful = false;
    bool TTPHideTrash = false;

    // TTSeriesWB: kept between sessions without saving a ttp profile
    bool SeriesWBEnabled = false;
    double SeriesWBMired = 0.0;
    double SeriesWBGreen = 1.0;
    double SeriesWBEqual = 1.0;
    bool SeriesWBFlashOnly = true;
    bool SeriesWBLearnTint = false;

    void readFromFile(Glib::KeyFile& keyFile);
    void saveToFile(Glib::KeyFile& keyFile) const;
};
