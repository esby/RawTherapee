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
#include "variable.h"

#include <cmath>
#include <cstdlib>
#include <string>

const char* rtVariableScopeName(RtVariableScope scope)
{
  switch (scope)
  {
    case RtVariableScope::Internal: return "internal";
    case RtVariableScope::Exif:     return "exif";
    case RtVariableScope::Image:    return "image";
    case RtVariableScope::Sequence: return "sequence";
    case RtVariableScope::Ancestor: return "ancestor";
    case RtVariableScope::Global:   return "global";
  }
  return "";
}

namespace
{

// strict parsing: the whole string must be a number, otherwise ok is false
double parseNumber(const Glib::ustring& s, bool& ok)
{
  const std::string str = s.raw();
  const char* begin = str.c_str();
  char* end = nullptr;
  double d = std::strtod(begin, &end);
  while (end != nullptr && (*end == ' ' || *end == '\t'))
    end++;
  ok = (end != begin) && (end != nullptr) && (*end == '\0');
  return ok ? d : 0.0;
}

}

RtVariable::RtVariable(Glib::ustring _name, Environment* _env)
  : name(_name), env(_env), scope(RtVariableScope::Internal)
{
  // the exif variables are recognised by their prefix (rti:...)
  if (name.compare(0, ROOT_EXIF_PREFIX.length() + 1, ROOT_EXIF_PREFIX + ":") == 0)
    scope = RtVariableScope::Exif;
}

Environment* RtVariable::getEnv()
{
  return env;
}

void RtVariable::setName(Glib::ustring _name)
{
  name = _name;
}

Glib::ustring RtVariable::getName()
{
  return name;
}

int RtVariable::getType()
{
  switch (value.index())
  {
    case 1: return RT_VARIABLE_TYPE_INT;
    case 2: return RT_VARIABLE_TYPE_DOUBLE;
    case 3: return RT_VARIABLE_TYPE_STRING;
    case 4: return RT_VARIABLE_TYPE_BOOL;
  }
  return RT_VARIABLE_TYPE_UNDEF;
}

bool RtVariable::isDefined()
{
  return value.index() != 0;
}

int RtVariable::getAsInt()
{
  if (auto p = std::get_if<int>(&value))
    return *p;
  if (auto p = std::get_if<double>(&value))
    return static_cast<int>(std::lround(*p));
  if (auto p = std::get_if<bool>(&value))
    return *p ? 1 : 0;
  if (auto p = std::get_if<Glib::ustring>(&value))
  {
    bool ok;
    double d = parseNumber(*p, ok);
    return ok ? static_cast<int>(std::lround(d)) : 0;
  }
  return 0;
}

double RtVariable::getAsDouble()
{
  if (auto p = std::get_if<double>(&value))
    return *p;
  if (auto p = std::get_if<int>(&value))
    return *p;
  if (auto p = std::get_if<bool>(&value))
    return *p ? 1.0 : 0.0;
  if (auto p = std::get_if<Glib::ustring>(&value))
  {
    bool ok;
    return parseNumber(*p, ok);
  }
  return 0.0;
}

Glib::ustring RtVariable::getAsString()
{
  // a string variable gives its value; the other types give their text form
  if (auto p = std::get_if<Glib::ustring>(&value))
    return *p;
  return toString();
}

bool RtVariable::getAsBool()
{
  if (auto p = std::get_if<bool>(&value))
    return *p;
  if (auto p = std::get_if<int>(&value))
    return *p != 0;
  if (auto p = std::get_if<double>(&value))
    return *p != 0.0;
  if (auto p = std::get_if<Glib::ustring>(&value))
  {
    Glib::ustring s = p->lowercase();
    return s == "true" || s == "yes" || s == "on" || s == "1";
  }
  return false;
}

void RtVariable::setAsInt(int v)
{
  value = v;
}

void RtVariable::setAsDouble(double v)
{
  value = v;
}

void RtVariable::setAsString(Glib::ustring v)
{
  value = v;
}

void RtVariable::setAsBool(bool v)
{
  value = v;
}

void RtVariable::clear()
{
  value = std::monostate();
}

Glib::ustring RtVariable::toString()
{
  if (auto p = std::get_if<Glib::ustring>(&value))
    return *p;
  if (auto p = std::get_if<int>(&value))
    return std::to_string(*p);
  if (auto p = std::get_if<double>(&value))
    return std::to_string(*p); // same text as before (6 decimals), the copy of the variables relies on it
  if (auto p = std::get_if<bool>(&value))
    return *p ? "true" : "false";
  return "";
}
