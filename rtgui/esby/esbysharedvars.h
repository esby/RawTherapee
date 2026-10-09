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

#include <memory>
#include <set>
#include <string>
#include <vector>
#include <glibmm.h>
#include <sigc++/sigc++.h>
#include "esbywbclient.h"

class Environment;

// shared variables of the opened image: the variables of the esby server (sequence, folders, global)
// are copied into the variables of the environment, with their scope and origin, so that the tools
// read them like the other variables (env->getVarAsDouble(...)). They are reloaded when the image,
// its sequence, or a variable concerning them changes (in this instance or in another one).
// A shared variable never replaces an internal or exif variable of the same name.
class EsbySharedVariables
{
public:
    // where a variable is set, from the opened image
    enum class Where { Sequence, Folder, Parent, Global };

    explicit EsbySharedVariables(Environment* env);
    ~EsbySharedVariables();

    // the opened image (its original file, see originalFile); nothing is done for the same file
    void setImage(const Glib::ustring& file);
    // the sequence of the image, as named by TTSeriesExposure (empty: the whole folder)
    void setSequence(const Glib::ustring& file, const Glib::ustring& sequence);

    Glib::ustring getFolder() const { return folder; }
    Glib::ustring getSequence() const { return sequence; }
    bool isConnected() const { return client && client->isConnected(); }
    // the variables of the current image and sequence are loaded (false while loading, or without server)
    bool isLoaded() const { return loadedOk; }
    // the connection to the server, for the other requests (file data, a variable of another folder)
    EsbyWBClient* getClient() { return client.get(); }

    void set(Where where, const EsbyVarValue& value);
    void unset(Where where, const Glib::ustring& name);
    // set or unset at a key returned by the server as origin (folder, folder#sequence, *)
    void setAt(const Glib::ustring& origin, const EsbyVarValue& value);
    void unsetAt(const Glib::ustring& origin, const Glib::ustring& name);

    // the shared variables were reloaded, or the connection changed
    sigc::signal<void>& signalChanged() { return changed; }

    // text typed by the user: true/false, integer, real number (. or ,), otherwise text
    static EsbyVarValue parseValue(const Glib::ustring& name, const Glib::ustring& text);
    // the original file of the opened image: ESBY_ORIGIN (set by a launcher such as rt_queue) when it
    // names the same file, otherwise the opened file
    static Glib::ustring originalFile(Environment* env);
    // the numbered folder of a photo: <folder>/pp and <folder>/pp/dpp belong to <folder>
    static Glib::ustring sequenceFolder(const Glib::ustring& file);
    static Glib::ustring realPath(const Glib::ustring& path);

private:
    void reload();
    void apply(const std::vector<EsbyVarValue>& variables);
    void clearLoaded();
    bool concerns(const Glib::ustring& path) const;
    void target(Where where, Glib::ustring& path, Glib::ustring& seq) const;

    Environment* env;
    std::unique_ptr<EsbyWBClient> client;
    Glib::ustring file;      // original file of the opened image
    Glib::ustring folder;    // its sequence folder, links resolved
    Glib::ustring sequence;  // part of the folder, empty for the whole folder
    std::set<std::string> loaded; // names of the variables copied into the environment
    int generation;          // an answer for a previous image or sequence is ignored
    bool loadedOk;
    sigc::signal<void> changed;
};
