
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
#if (BOOST_VERSION>=106500)
#include "boost/python/numpy.hpp"
#else
#include "boost/python/numeric.hpp"
#endif




namespace insight
{

    
    
    
acquire_py_GIL::acquire_py_GIL()
{
    state = PyGILState_Ensure();
}




acquire_py_GIL::~acquire_py_GIL()
{

    PyGILState_Release(state);
}

    
    
    
    
void PythonInterpreter::startInterpreter()
{
    if (!Py_IsInitialized())
    {
        Py_Initialize();
        PyEval_InitThreads();
        mainThreadState = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
        ranInitialize_=true;
    }
    else
    {
        ranInitialize_=false;
    }
}

PythonInterpreter::PythonInterpreter()
: mainThreadState(nullptr),
  ranInitialize_(false)
{
    startInterpreter();
}




PythonInterpreter::~PythonInterpreter()
{
    if (ranInitialize_ && Py_IsInitialized())
    {
        PyEval_RestoreThread(mainThreadState); // Py_END_ALLOW_THREADS
        Py_Finalize();
    }
}


PythonInterpreter pythonInterpreter;
    
}
