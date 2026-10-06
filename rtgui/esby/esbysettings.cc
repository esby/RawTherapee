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
#include "esbysettings.h"
#include <glibmm/miscutils.h>
#include "options.h" // argv0

Glib::ustring paramFileGuiExtension = ".ttp";

Glib::ustring esbyTranslationFile()
{
    // installed by rtgui/esby/esby.cmake, outside the languages folder:
    // every file of the languages folder is listed as a language in the preferences.
    return Glib::build_filename(argv0, "esby", "languages", "default");
}

// called by Options::readFromFile(), the code was moved from options.cc
void EsbySettings::readFromFile(Glib::KeyFile& keyFile)
{
            if (keyFile.has_group ("TTP")) {
                if (keyFile.has_key ("TTP", "Autoload")) {
                    TTPAutoload       = keyFile.get_boolean ("TTP", "Autoload");
                }

                if (keyFile.has_key ("TTP", "AutoloadValue")) {
                    TTPAutoloadValue      = keyFile.get_string ("TTP", "AutoloadValue");
                }

                if (keyFile.has_key ("TTP", "HideFavorite")) {
                    TTPHideFavorite       = keyFile.get_boolean ("TTP", "HideFavorite");
                }

                if (keyFile.has_key ("TTP", "HideExposure")) {
                    TTPHideExposure       = keyFile.get_boolean ("TTP", "HideExposure");
                }

                if (keyFile.has_key ("TTP", "HideDetails")) {
                    TTPHideDetails        = keyFile.get_boolean ("TTP", "HideDetails");
                }

                if (keyFile.has_key ("TTP", "HideColor")) {
                    TTPHideColor          = keyFile.get_boolean ("TTP", "HideColor");
                }

                if (keyFile.has_key ("TTP", "HideAdvanced")) {
                    TTPHideAdvanced       = keyFile.get_boolean ("TTP", "HideAdvanced");
                }

                if (keyFile.has_key ("TTP", "HideLocal")) {
                    TTPHideLocal       = keyFile.get_boolean ("TTP", "HideLocal");
                }

                if (keyFile.has_key ("TTP", "HideTransform")) {
                    TTPHideTransform       = keyFile.get_boolean ("TTP", "HideTransform");
                }

                if (keyFile.has_key ("TTP", "HideRaw")) {
                    TTPHideRaw             = keyFile.get_boolean ("TTP", "HideRaw");
                }

                if (keyFile.has_key ("TTP", "HideMetadata")) {
                    TTPHideMetadata        = keyFile.get_boolean ("TTP", "HideMetadata");
                }

                if (keyFile.has_key ("TTP", "HideUseful")) {
                    TTPHideUseful         = keyFile.get_boolean ("TTP", "HideUseful");
                }

                if (keyFile.has_key ("TTP", "HideTrash")) {
                    TTPHideTrash          = keyFile.get_boolean ("TTP", "HideTrash");
                }
            }
}

// called by Options::saveToFile(), the code was moved from options.cc
void EsbySettings::saveToFile(Glib::KeyFile& keyFile) const
{
        keyFile.set_boolean ("TTP", "Autoload", TTPAutoload);
        keyFile.set_string  ("TTP", "AutoloadValue", TTPAutoloadValue);
        keyFile.set_boolean ("TTP", "HideFavorite", TTPHideFavorite);
        keyFile.set_boolean ("TTP", "HideExposure", TTPHideExposure);
        keyFile.set_boolean ("TTP", "HideDetails", TTPHideDetails);
        keyFile.set_boolean ("TTP", "HideColor", TTPHideColor);
        keyFile.set_boolean ("TTP", "HideAdvanced", TTPHideAdvanced);
        keyFile.set_boolean ("TTP", "HideLocal", TTPHideLocal);
        keyFile.set_boolean ("TTP", "HideTransform", TTPHideTransform);
        keyFile.set_boolean ("TTP", "HideRaw", TTPHideRaw);
        keyFile.set_boolean ("TTP", "HideMetadata", TTPHideMetadata);
        keyFile.set_boolean ("TTP", "HideUseful", TTPHideUseful);
        keyFile.set_boolean ("TTP", "HideTrash", TTPHideTrash);
}
