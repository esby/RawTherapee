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

#include <cstdio>
#include "options.h"

// console traces of the esby tools (favorites, moves, tt*), only displayed in verbose mode.
// TT_LOG is used like printf, and can be used anywhere a statement is expected (ex: after an else).
// error messages are not concerned: they are always displayed.
#define TT_VERBOSE (options.rtSettings.verbose)
#define TT_LOG(...) do { if (TT_VERBOSE) printf(__VA_ARGS__); } while (0)
