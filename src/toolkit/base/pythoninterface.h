
/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#ifndef INSIGHT_PYTHONINTERFACE_H
#define INSIGHT_PYTHONINTERFACE_H

#include "Python.h"

// class _ts;
// 
// typedef struct _ts PyThreadState;

namespace insight
{
    
struct aquire_py_GIL 
{
    PyGILState_STATE state;
    
    aquire_py_GIL();
    ~aquire_py_GIL();
};


struct release_py_GIL 
{
    PyThreadState *state;
    
    release_py_GIL();
    ~release_py_GIL();
};


class PythonInterpreter
{
  PyThreadState* mainThreadState;
  bool ranInitialize_;
  void startInterpreter();
  
public:
    PythonInterpreter();
    ~PythonInterpreter();
};

extern PythonInterpreter pythonInterpreter;

}

#endif
