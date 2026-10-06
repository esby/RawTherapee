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
#ifndef __RT_VARIABLE__
#define __RT_VARIABLE__

#include <gtkmm.h>
#include <glibmm.h>
#include "../rtengine/rtengine.h"
#include "../rtengine/procparams.h"
#include "rtdef.h"
#include "environment.h"

#define RT_VARIABLE_TYPE_UNDEF -1
#define RT_VARIABLE_TYPE_INT 0
#define RT_VARIABLE_TYPE_DOUBLE 1
#define RT_VARIABLE_TYPE_STRING 2
#define RT_VARIABLE_TYPE_BOOL 3

class RtVariableStore 
{
  public:
    RtVariableStore()    {    }
    virtual ~RtVariableStore()    { }

    virtual int getAsInt();
    virtual double getAsDouble();
    virtual Glib::ustring getAsString();
    virtual bool getAsBool();

    virtual void setAsInt(int v);
    virtual void setAsDouble(double v);
    virtual void setAsString(Glib::ustring v);
    virtual void setAsBool(bool v);
};

class RtVariableIntStore : public RtVariableStore
{
  protected:
    int value;
  public:
    int getAsInt();
    void setAsInt(int v);
};

class RtVariableDoubleStore : public RtVariableStore
{
  protected:
    double value;
  public:
    double getAsDouble();
    void setAsDouble(double v);
};

class RtVariableStringStore : public RtVariableStore
{
  protected:
    Glib::ustring value;
  public:
    Glib::ustring getAsString(); 
    void setAsString(Glib::ustring v);
};

class RtVariableBoolStore : public RtVariableStore
{
  protected:
    bool value;
  public:
    bool getAsBool(); 
    void setAsBool(bool v);
};

// this is the container that should be used
class RtVariable
{

  protected:
   Glib::ustring name;
   Environment* env;
   int type;
   RtVariableStore* value;

  public:
    RtVariable(Glib::ustring _name, Environment* _env);
    ~RtVariable() ;

    Environment* getEnv();

    void setName(Glib::ustring _name);
    Glib::ustring getName();

    int getAsInt() ;
    double getAsDouble();
    Glib::ustring getAsString();
    bool getAsBool();

    int getType() ;

    void setAsInt(int v) ;
    void setAsDouble(double v);
    void setAsString(Glib::ustring v);
    void setAsBool(bool v);

    Glib::ustring toString();  
    
};


#endif
