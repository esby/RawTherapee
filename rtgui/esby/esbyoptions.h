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

#include "options.h"

// access to the RawTherapee options from the esby code.
// upstream replaced the global variable 'options' by the App singleton
// (App::get().options() to read, App::get().mut_options() to modify):
// after the merge, only this function has to change, ex:
//   inline Options& esbyOptions() { return App::get().mut_options(); }
inline Options& esbyOptions()
{
    return options;
}

// options of the esby fork (EsbySettings, stored in the [TTP] group), member of Options
inline EsbySettings& esbySettings()
{
    return esbyOptions().esby;
}
