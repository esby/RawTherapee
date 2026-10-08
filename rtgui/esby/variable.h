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
#ifndef __RT_VARIABLE__
#define __RT_VARIABLE__

#include <variant>
#include <glibmm.h>
#include "rtdef.h"
#include "environment.h" // the tools including variable.h use the environment too

#define RT_VARIABLE_TYPE_UNDEF -1
#define RT_VARIABLE_TYPE_INT 0
#define RT_VARIABLE_TYPE_DOUBLE 1
#define RT_VARIABLE_TYPE_STRING 2
#define RT_VARIABLE_TYPE_BOOL 3

// where the value of a variable comes from, and how far it is shared.
// internal and exif variables belong to the opened image and are never shared;
// the other scopes are meant to be resolved and shared by the esby server
// (the nearest scope wins: image, then sequence, then ancestor folder, then global).
enum class RtVariableScope
{
  Internal, // set by the code (Fname, Iso, pp3version...)
  Exif,     // read from the metadata of the image (rti:Exif:...)
  Image,    // set for this image only
  Sequence, // shared by the images of a sequence
  Ancestor, // set on an ancestor folder, inherited by its sequences
  Global    // shared by every image
};

const char* rtVariableScopeName(RtVariableScope scope);

// a named value, typed by the last setter used.
// the getters convert between the types (an int variable read as a double gives its value,
// a string variable read as a number is parsed), an undefined variable gives 0, "" or false.
// the environment holds pointers to the variables: they are neither copied nor moved.
class RtVariable
{
  protected:
    Glib::ustring name;
    Environment* env;
    std::variant<std::monostate, int, double, Glib::ustring, bool> value;
    RtVariableScope scope;
    Glib::ustring origin; // folder the value comes from, empty when not relevant

  public:
    RtVariable(Glib::ustring _name, Environment* _env);
    ~RtVariable() = default;
    RtVariable(const RtVariable&) = delete;
    RtVariable& operator=(const RtVariable&) = delete;

    Environment* getEnv();

    void setName(Glib::ustring _name);
    Glib::ustring getName();

    int getAsInt();
    double getAsDouble();
    Glib::ustring getAsString();
    bool getAsBool();

    int getType();
    bool isDefined();

    void setAsInt(int v);
    void setAsDouble(double v);
    void setAsString(Glib::ustring v);
    void setAsBool(bool v);
    void clear(); // back to undefined

    RtVariableScope getScope() { return scope; }
    void setScope(RtVariableScope s) { scope = s; }
    Glib::ustring getOrigin() { return origin; }
    void setOrigin(const Glib::ustring& o) { origin = o; }

    Glib::ustring toString();
};

#endif
