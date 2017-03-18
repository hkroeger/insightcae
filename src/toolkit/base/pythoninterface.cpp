
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

#include "pythoninterface.h"

#include <base/exception.h>


#include "boost/python.hpp"
#include "boost/python/numeric.hpp"




namespace insight
{

    
    
    
aquire_py_GIL::aquire_py_GIL() 
{
    state = PyGILState_Ensure();
}




aquire_py_GIL::~aquire_py_GIL() 
{
    PyGILState_Release(state);
}




release_py_GIL::release_py_GIL() 
{
    state = PyEval_SaveThread();
}



release_py_GIL::~release_py_GIL() 
{
    PyEval_RestoreThread(state);
}
    
    
    
    
void PythonInterpreter::startInterpreter()
{
    if (!Py_IsInitialized())
    {
        Py_Initialize();
        PyEval_InitThreads();
        mainThreadState = PyEval_SaveThread();
        ranInitialize_=true;
    }
    else
    {
        ranInitialize_=false;
    }
}

PythonInterpreter::PythonInterpreter()
: mainThreadState(NULL),
  ranInitialize_(false)
{
    startInterpreter();
}




PythonInterpreter::~PythonInterpreter()
{
    if (ranInitialize_ && Py_IsInitialized())
    {
        PyEval_RestoreThread(mainThreadState);
        Py_Finalize();
    }
}


PythonInterpreter pythonInterpreter;
    
}
