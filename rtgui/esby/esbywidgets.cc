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
#include "esbywidgets.h"
#include "guiutils.h"

void esbyInstallCompactCss()
{
    static bool installed = false;
    if (installed)
        return;
    Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
    if (!screen)
        return;
    Glib::RefPtr<Gtk::CssProvider> css = Gtk::CssProvider::create();
    try
    {
        css->load_from_data("button." ESBY_COMPACT_CLASS " { padding: 0px 3px; min-width: 0px; min-height: 0px; }");
    }
    catch (const Glib::Error&)
    {
        return;
    }
    // above the priority of the themes, which set the paddings of the buttons
    Gtk::StyleContext::add_provider_for_screen(screen, css, GTK_STYLE_PROVIDER_PRIORITY_USER + 1);
    installed = true;
}

Gtk::Label* esbyFindLabel(Gtk::Widget* widget)
{
    if (widget == nullptr)
        return nullptr;
    if (Gtk::Label* label = dynamic_cast<Gtk::Label*>(widget))
        return label;
    if (Gtk::Container* container = dynamic_cast<Gtk::Container*>(widget))
    {
        for (Gtk::Widget* child : container->get_children())
        {
            if (Gtk::Label* label = esbyFindLabel(child))
                return label;
        }
    }
    return nullptr;
}

void esbyMakeTitleShrinkable(MyExpander* expander)
{
    if (expander == nullptr)
        return;
    Gtk::Label* label = esbyFindLabel(expander->getLabelWidget());
    if (label == nullptr)
        return;
    label->set_line_wrap(false);
    label->set_ellipsize(Pango::ELLIPSIZE_END);
    label->set_width_chars(6); // a few characters stay readable
    if (label->get_tooltip_text().empty())
        label->set_tooltip_text(label->get_text());
}

void esbyWrapLabels(Gtk::Widget* root)
{
    if (root == nullptr)
        return;
    if (Gtk::Label* label = dynamic_cast<Gtk::Label*>(root))
    {
        if (label->get_ellipsize() == Pango::ELLIPSIZE_NONE)
        {
            label->set_line_wrap(true);
            label->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        }
        return;
    }
    if (Gtk::Container* container = dynamic_cast<Gtk::Container*>(root))
    {
        for (Gtk::Widget* child : container->get_children())
            esbyWrapLabels(child);
    }
}
