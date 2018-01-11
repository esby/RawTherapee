/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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

using namespace rtengine::procparams;


// RtVariableStore dummy implementation
int RtVariableStore::getAsInt() { return -1;}
double RtVariableStore::getAsDouble(){ return -1;}
Glib::ustring RtVariableStore::getAsString(){return "";}
bool RtVariableStore::getAsBool(){return false;}

void RtVariableStore::setAsInt(int v){}
void RtVariableStore::setAsDouble(double v){}
void RtVariableStore::setAsString(Glib::ustring v){}
void RtVariableStore::setAsBool(bool v){}


// class RtVariableIntStore 
int RtVariableIntStore::getAsInt()
{
  return value;
}

void RtVariableIntStore::setAsInt(int v)
{
  value = v;
}
// class RtVariableDoubleStore
double RtVariableDoubleStore::getAsDouble()
{
  return value;
}

void RtVariableDoubleStore::setAsDouble(double v)
{
  value = v;
}

// class RtVariableStringStore
Glib::ustring RtVariableStringStore::getAsString()
{
  return value;
}

void RtVariableStringStore::setAsString(Glib::ustring v)
{
  //todo: should the string be copied ?
  value = v;
}

// class RtVariableBoolStore 
bool RtVariableBoolStore::getAsBool()
{
  return value;
}

void RtVariableBoolStore::setAsBool(bool v)
{
  value = v;
}


// class RtVariable
RtVariable::RtVariable(Glib::ustring _name, Environment* _env)
{
  type = RT_VARIABLE_TYPE_UNDEF;
  value = nullptr;
  name = _name;
  env = _env;
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

int RtVariable::getAsInt() 
{ 
  if (type == RT_VARIABLE_TYPE_INT ) 
    return static_cast<RtVariableIntStore*>(value)->getAsInt(); 
  return 0;
}

double RtVariable::getAsDouble() 
{  
  if (type == RT_VARIABLE_TYPE_DOUBLE ) 
    return static_cast<RtVariableDoubleStore*>(value)->getAsDouble() ; 
  return 0;
}

Glib::ustring RtVariable::getAsString() 
{  
  if (type == RT_VARIABLE_TYPE_STRING ) 
    return static_cast<RtVariableStringStore*>(value)->getAsString() ; 
  return "";
}

bool RtVariable::getAsBool() 
{  
  if (type == RT_VARIABLE_TYPE_BOOL ) 
    return static_cast<RtVariableBoolStore*>(value)->getAsBool() ; 
  return false;
}

int RtVariable::getType() 
{ 
  return type;
}

void RtVariable::setAsInt(int v)
{
  if (type != RT_VARIABLE_TYPE_INT)
  {
if (value!= nullptr) delete value;
value = new RtVariableIntStore();
type = RT_VARIABLE_TYPE_INT;
  }
  value->setAsInt(v);
}

void RtVariable::setAsDouble(double v)
{
  if (type != RT_VARIABLE_TYPE_DOUBLE)
  {
if (value!= nullptr) delete value;
value = new RtVariableDoubleStore();
type = RT_VARIABLE_TYPE_DOUBLE;
  }
  value->setAsDouble(v);
}

void RtVariable::setAsString(Glib::ustring v)
{
  if (type != RT_VARIABLE_TYPE_STRING)
  {
if (value!= nullptr) delete value;
value = new RtVariableStringStore();
type = RT_VARIABLE_TYPE_STRING;
  }
  value->setAsString(v);
}


void RtVariable::setAsBool(bool v)
{
  if (type != RT_VARIABLE_TYPE_BOOL)
  {
if (value!= nullptr) delete value;
value = new RtVariableBoolStore();
type = RT_VARIABLE_TYPE_BOOL;
 }
  value->setAsBool(v);
   }

Glib::ustring RtVariable::toString() 
{
  if (type == RT_VARIABLE_TYPE_STRING) return getAsString();
  if (type == RT_VARIABLE_TYPE_INT)
  {  
 return g_strdup_printf("%i", getAsInt());
  }

  if (type == RT_VARIABLE_TYPE_DOUBLE)
  {
 return g_strdup_printf("%f", getAsDouble());
  }

  if (type == RT_VARIABLE_TYPE_BOOL)
 return getAsBool() ? "true" : "false";
  return "";
}



