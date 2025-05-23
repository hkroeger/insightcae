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


#include "pythonanalysis.h"
#include "exception.h"

#include <fstream>
#include <cstdlib>
#include <dlfcn.h>
#include <functional>

#include "base/boost_include.h"
#include "boost/function.hpp"
#include "boost/python.hpp"

#include "base/pythoninterface.h"
#include "swigpyrun.h"
#define SWIG_as_voidptr(a) const_cast< void * >(static_cast< const void * >(a))

using namespace std;
using namespace boost;
using namespace boost::filesystem;


namespace insight
{



using namespace boost::python;






std::unique_ptr<ParameterSet> PythonAnalysis::defaultParameters(
    const boost::filesystem::path& scriptfile )
{
    acquire_py_GIL locker;
    
    try
    {
        object main_module(handle<>(borrowed(PyImport_AddModule("__main__"))));
        object main_namespace = main_module.attr("__dict__");
        handle<> ignore(PyRun_String(
            boost::str( boost::format(
                           "import imp; mod = imp.load_source('module', '%s'); ps=mod.defaultParameters()"
                           ) % scriptfile.string() ).c_str(),
            Py_file_input,
            main_namespace.ptr(),
            main_namespace.ptr() )
        );
        object o =  extract<object>(main_namespace["ps"]);
        ParameterSet *psp;
        static void *descr = nullptr;
        if (!descr)
        {
            descr = SWIG_TypeQuery("insight::ParameterSet *");    /* Get the type descriptor structure for Foo */
            std::cout<<descr<<std::endl;
            assert(descr);
        }
        if ((SWIG_ConvertPtr(o.ptr(), (void **) &psp, descr, 0) == -1))
        {
            abort();
        }
        return std::unique_ptr<ParameterSet>(psp);
    }
    catch (const error_already_set &)
    {
        PyErr_Print();
        return ParameterSet::create();
    }
}




std::string PythonAnalysis::category(
    const boost::filesystem::path& scriptfile )
{
    acquire_py_GIL locker;
    
    try
    {
        object main_module(handle<>(borrowed(PyImport_AddModule("__main__"))));
        object main_namespace = main_module.attr("__dict__");
        handle<> ignore(PyRun_String(
            boost::str( boost::format(
                           "import imp; mod = imp.load_source('module', '%s'); cat=mod.category()"
                           ) % scriptfile.string() ).c_str(),
            Py_file_input,
            main_namespace.ptr(),
            main_namespace.ptr() )
        );
        return std::string( extract<std::string>(main_namespace["cat"]) );
    }
    catch (const error_already_set &)
    {
        PyErr_Print();
        return "With Error";
    }
}



PythonAnalysis::PythonAnalysis(
    const boost::filesystem::path& scriptfile,
    const std::shared_ptr<supplementedInputDataBase>& sid )
: Analysis(sid),
  scriptfile_(scriptfile)
{}
 
 
ResultSetPtr PythonAnalysis::operator() ( ProgressDisplayer& )
{
    acquire_py_GIL locker;
    
    try
    {
        
        static void *descr = nullptr;
        if (!descr) {
            descr = SWIG_TypeQuery("insight::ParameterSet *");    /* Get the type descriptor structure for Foo */
            assert(descr);
        }
        
        PyObject *paramobj;
        if (!(paramobj = SWIG_NewPointerObj(SWIG_as_voidptr(&parameters()), descr, 0 )))
        {
            throw insight::Exception("Could not create python parameter object!");
        }
        
        
        object main_module(handle<>(borrowed(PyImport_AddModule("__main__"))));
        object main_namespace = main_module.attr("__dict__");
        main_module.attr("parameters")=python::borrowed(paramobj);
        main_module.attr("workdir")=executionPath().string();
        
        handle<> ignore(PyRun_String(
            boost::str( boost::format(
                "import imp;"
                "mod = imp.load_source('module', '%s');"
                "result=mod.executeAnalysis(parameters,workdir)"
            ) % scriptfile_.string() ).c_str(),
            Py_file_input,
            main_namespace.ptr(),
            main_namespace.ptr() )
        );
        
        object o =  extract<object>(main_namespace["result"]);
        ResultSetPtr *res;
        descr=nullptr;
        if (!descr) {
            descr = SWIG_TypeQuery("insight::ResultSetPtr *");    /* Get the type descriptor structure for Foo */
            assert(descr);
        }
        if ((SWIG_ConvertPtr(o.ptr(), (void **) &res, descr, 0) == -1)) {
            throw insight::Exception("Could not convert return value!");
        }
        return ResultSetPtr(*res);
    }
    catch (const error_already_set &)
    {
        PyErr_Print();
        return ResultSetPtr();
    }
}





    

PythonAnalysisLoader::PythonAnalysisLoader()
{
    auto paths = SharedPathList::global();
    for ( auto& p: paths )
    {
        if ( exists(p) && is_directory ( p ) )
        {
            path pydir ( p );
            pydir /= "python_modules";
            if ( exists(pydir) )
            {
                if (is_directory ( pydir ) )
                {
                    directory_iterator end_itr; // default construction yields past-the-end
                    for ( directory_iterator itr ( pydir );
                        itr != end_itr;
                        ++itr ) {
                    if ( is_regular_file ( itr->status() ) )
                    {
                        if ( itr->path().extension() == ".py" )
                        {
                            auto scriptfile = itr->path();
                            std::string analysisName = scriptfile.string();
                            
                            Analysis::descriptions().emplace(
                                analysisName,
                                [scriptfile](){ return AnalysisDescription{ scriptfile.string(), "" }; }
                                );

                            Analysis::categories().emplace(
                                analysisName,
                                [scriptfile](){ return PythonAnalysis::category(scriptfile); }
                                );

                            Analysis::defaultParameters().emplace(
                                analysisName,
                                [scriptfile](){ return PythonAnalysis::defaultParameters(scriptfile); }
                                );


                            Analysis::supplementedInputDatas().emplace(
                                analysisName,
                                [](ParameterSetInput ip,
                                   const boost::filesystem::path& exePath,
                                   ProgressDisplayer& pd)
                                { return std::make_unique<supplementedInputDataBase>(std::move(ip), exePath, pd); }
                                );

                            Analysis::analyses().emplace(
                                analysisName,
                                [scriptfile](const std::shared_ptr<supplementedInputDataBase>& sp)
                                { return std::make_unique<PythonAnalysis>(scriptfile, sp); }
                            );
                            
                        }
                    }
                }
            }}

        } else {
            //cout<<"Not existing: "<<p<<endl;
        }
    }
}

PythonAnalysisLoader::~PythonAnalysisLoader()
{
}



PythonAnalysisLoader pyloader;

}
