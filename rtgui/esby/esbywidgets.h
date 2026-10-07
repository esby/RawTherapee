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

#include <gtkmm.h>

class MyExpander;

// layout helpers of the esby fork: the tool panel must fit in a narrow side panel.
// A Gtk::Notebook takes the minimal width of its widest page: one wide header or one long
// label in any tab made the whole panel wider than the screen side panel.

// style class of the compact buttons (favorite, trash, moves): small padding, no minimal size
#define ESBY_COMPACT_CLASS "esby-compact"
void esbyInstallCompactCss();

// the title of a tool header can be shortened with "..." (full title in the tooltip)
void esbyMakeTitleShrinkable(MyExpander* expander);

// the labels inside a widget wrap (anywhere if needed, ex: long paths)
void esbyWrapLabels(Gtk::Widget* root);

// label of a header: the header itself, or the first label inside it
Gtk::Label* esbyFindLabel(Gtk::Widget* widget);
