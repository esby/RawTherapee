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
#include "esbysharedvars.h"
#include "variable.h"
#include "ttlog.h"

#include <cstdlib>
#include <cmath>

static const char GLOBAL_KEY[] = "*";

EsbySharedVariables::EsbySharedVariables(Environment* _env) :
    env(_env),
    generation(0)
{
    client.reset(new EsbyWBClient());
    client->setStatusCallback([this](bool connected) {
        if (connected)
            reload();
        changed.emit();
    });
    client->setVariableEventCallback([this](const Glib::ustring& path, const Glib::ustring&) {
        if (concerns(path))
            reload();
    });
    client->start();
}

EsbySharedVariables::~EsbySharedVariables()
{
}

Glib::ustring EsbySharedVariables::realPath(const Glib::ustring& path)
{
    char* resolved = realpath(path.c_str(), nullptr);
    if (resolved == nullptr)
        return path;
    Glib::ustring result(resolved);
    free(resolved);
    return result;
}

Glib::ustring EsbySharedVariables::originalFile(Environment* env)
{
    if (env == nullptr)
        return "";
    Glib::ustring file = env->getVarAsString("Fname");
    const char* origin = g_getenv("ESBY_ORIGIN");
    if ((origin != nullptr) && (*origin != '\0') && !file.empty()
    && (Glib::path_get_basename(origin) == Glib::path_get_basename(file)))
        return origin;
    return file;
}

Glib::ustring EsbySharedVariables::sequenceFolder(const Glib::ustring& file)
{
    std::string dir = Glib::path_get_dirname(file);
    if ((Glib::path_get_basename(dir) == "dpp") && (Glib::path_get_basename(Glib::path_get_dirname(dir)) == "pp"))
        return Glib::path_get_dirname(Glib::path_get_dirname(dir));
    if (Glib::path_get_basename(dir) == "pp")
        return Glib::path_get_dirname(dir);
    return dir;
}

void EsbySharedVariables::setImage(const Glib::ustring& _file)
{
    if (_file == file)
        return;
    file = _file;
    folder = file.empty() ? Glib::ustring() : realPath(sequenceFolder(file));
    sequence.clear();
    clearLoaded(); // the values of the previous image must not be seen with this one
    reload();
}

void EsbySharedVariables::setSequence(const Glib::ustring& _file, const Glib::ustring& _sequence)
{
    setImage(_file);
    if (_sequence == sequence)
        return;
    sequence = _sequence;
    reload();
}

void EsbySharedVariables::reload()
{
    int current = ++generation;
    if (folder.empty() || !isConnected())
    {
        clearLoaded();
        changed.emit();
        return;
    }
    client->varGet(folder, sequence, [this, current](bool ok, const std::vector<EsbyVarValue>& variables) {
        if (current != generation)
            return; // the image or the sequence changed meanwhile
        if (ok)
            apply(variables);
        else
            clearLoaded();
        changed.emit();
    });
}

static RtVariableScope scopeOf(const Glib::ustring& scope)
{
    if (scope == "sequence")
        return RtVariableScope::Sequence;
    if (scope == "ancestor")
        return RtVariableScope::Ancestor;
    return RtVariableScope::Global;
}

void EsbySharedVariables::apply(const std::vector<EsbyVarValue>& variables)
{
    std::set<std::string> previous;
    previous.swap(loaded);
    for (const EsbyVarValue& v : variables)
    {
        RtVariable* var = env->getVariableByName(v.name);
        if ((var != nullptr) && var->isDefined() && (previous.count(v.name) == 0)
        && ((var->getScope() == RtVariableScope::Internal) || (var->getScope() == RtVariableScope::Exif)))
        {
            TT_LOG("shared variable %s ignored: an internal variable has this name\n", v.name.c_str());
            continue;
        }
        var = env->findOrCreateVariable(v.name);
        switch (v.type)
        {
            case RT_VARIABLE_TYPE_INT:    var->setAsInt(v.i); break;
            case RT_VARIABLE_TYPE_DOUBLE: var->setAsDouble(v.d); break;
            case RT_VARIABLE_TYPE_BOOL:   var->setAsBool(v.b); break;
            default:                      var->setAsString(v.s); break;
        }
        var->setScope(scopeOf(v.scope));
        var->setOrigin(v.origin);
        loaded.insert(v.name);
        previous.erase(v.name);
    }
    // the variables unset since the previous load
    for (const std::string& name : previous)
    {
        RtVariable* var = env->getVariableByName(name);
        if (var != nullptr)
        {
            var->clear();
            var->setOrigin("");
        }
    }
}

void EsbySharedVariables::clearLoaded()
{
    for (const std::string& name : loaded)
    {
        RtVariable* var = env->getVariableByName(name);
        if (var != nullptr)
        {
            var->clear();
            var->setOrigin("");
        }
    }
    loaded.clear();
}

// a change at path concerns the image: global, its folder (any sequence of it), or a folder above it
bool EsbySharedVariables::concerns(const Glib::ustring& path) const
{
    if (folder.empty())
        return false;
    if (path == GLOBAL_KEY)
        return true;
    Glib::ustring changedFolder = path;
    Glib::ustring::size_type hash = path.find('#');
    if (hash != Glib::ustring::npos)
        changedFolder = path.substr(0, hash);
    if (changedFolder == folder)
        return true;
    return (changedFolder == "/") || (folder.compare(0, changedFolder.length() + 1, changedFolder + "/") == 0);
}

void EsbySharedVariables::target(Where where, Glib::ustring& path, Glib::ustring& seq) const
{
    seq.clear();
    switch (where)
    {
        case Where::Sequence:
            path = folder;
            seq = sequence;
            break;
        case Where::Folder:
            path = folder;
            break;
        case Where::Parent:
            path = Glib::path_get_dirname(folder);
            break;
        case Where::Global:
            path = "global";
            break;
    }
}

void EsbySharedVariables::set(Where where, const EsbyVarValue& value)
{
    if (folder.empty() && (where != Where::Global))
        return;
    Glib::ustring path, seq;
    target(where, path, seq);
    client->varSet(path, seq, value, nullptr); // the event of the server reloads the variables
}

void EsbySharedVariables::unset(Where where, const Glib::ustring& name)
{
    if (folder.empty() && (where != Where::Global))
        return;
    Glib::ustring path, seq;
    target(where, path, seq);
    client->varUnset(path, seq, name, nullptr);
}

static void splitOrigin(const Glib::ustring& origin, Glib::ustring& path, Glib::ustring& seq)
{
    seq.clear();
    if (origin == GLOBAL_KEY)
    {
        path = "global";
        return;
    }
    Glib::ustring::size_type hash = origin.find('#');
    path = origin.substr(0, hash);
    if (hash != Glib::ustring::npos)
        seq = origin.substr(hash + 1);
}

void EsbySharedVariables::setAt(const Glib::ustring& origin, const EsbyVarValue& value)
{
    Glib::ustring path, seq;
    splitOrigin(origin, path, seq);
    client->varSet(path, seq, value, nullptr);
}

void EsbySharedVariables::unsetAt(const Glib::ustring& origin, const Glib::ustring& name)
{
    Glib::ustring path, seq;
    splitOrigin(origin, path, seq);
    client->varUnset(path, seq, name, nullptr);
}

EsbyVarValue EsbySharedVariables::parseValue(const Glib::ustring& name, const Glib::ustring& text)
{
    EsbyVarValue v;
    v.name = name;
    Glib::ustring low = text.lowercase();
    if ((low == "true") || (low == "false"))
    {
        v.type = RT_VARIABLE_TYPE_BOOL;
        v.b = (low == "true");
        return v;
    }
    std::string number = text.raw();
    for (char& c : number)
        if (c == ',')
            c = '.'; // a French decimal comma
    const char* begin = number.c_str();
    char* end = nullptr;
    long l = std::strtol(begin, &end, 10);
    if ((end != begin) && (*end == '\0') && (std::labs(l) < 1000000000L))
    {
        v.type = RT_VARIABLE_TYPE_INT;
        v.i = (int) l;
        return v;
    }
    double d = g_ascii_strtod(begin, &end);
    if ((end != begin) && (*end == '\0') && std::isfinite(d))
    {
        v.type = RT_VARIABLE_TYPE_DOUBLE;
        v.d = d;
        return v;
    }
    v.type = RT_VARIABLE_TYPE_STRING;
    v.s = text;
    return v;
}
