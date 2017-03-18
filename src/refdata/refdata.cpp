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

#include "refdata.h"
#include <base/exception.h>

#include "Python.h"
#include "boost/python.hpp"
#include "boost/python/numeric.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::python;

namespace insight
{




  
void ReferenceDataLibrary::findDataSets()
{
    if (!datasetsloaded_)
    {
        aquire_py_GIL locker;

        try
        {

            object main_module(handle<>(borrowed(PyImport_AddModule("__main__"))));
            object main_namespace = main_module.attr("__dict__");
            handle<> ignore(PyRun_String( "import Insight.ReferenceData as mod; modpath=mod.__file__;",
                                        Py_file_input,
                                        main_namespace.ptr(),
                                        main_namespace.ptr() ));
            
            std::string mp = extract<std::string>(main_namespace["modpath"]);

            path dir = path(mp).parent_path();

            if ( exists( dir ) )
            {
                directory_iterator end_itr; // default construction yields past-the-end
                for ( directory_iterator itr( dir );
                        itr != end_itr;
                        ++itr )
                {
                    if ( is_directory(itr->status()) )
                    {
                        path mod=itr->path()/"__init__.py";
                        if (exists(mod))
                        {
                            datasets_[itr->path().filename().string()]=mod;
                            // 	  cout<<"Added "<<itr->path().filename().string()<<": "<<mod<<endl;
                        }
                    }
                }
            }
            datasetsloaded_=true;
        }
        catch (const error_already_set &)
        {
            PyErr_Print();
        }
        
    }
}


ReferenceDataLibrary::ReferenceDataLibrary()
: datasetsloaded_(false)
{}


ReferenceDataLibrary::~ReferenceDataLibrary()
{}
 

arma::mat ReferenceDataLibrary::getProfile(const std::string& dataSetName, const std::string& path) const
{
    const_cast<ReferenceDataLibrary*>(this)->findDataSets();

    aquire_py_GIL locker;

    arma::mat profile;
    try
    {

        DataSetList::const_iterator i=datasets_.find(dataSetName);
        if (i==datasets_.end())
        {
            cout<<"Dataset "<<dataSetName<<" not found in library! Returning empty data array."<<endl;
            return arma::zeros(1,2);
        }

        std::ostringstream cmd;
        cmd<<
           "import Insight.ReferenceData."<< dataSetName <<" as mod;"
           "result=mod.getProfile('"<<path<<"')";
        cout<<cmd.str()<<endl;

        object main_module(handle<>(borrowed(PyImport_AddModule("__main__"))));
        object main_namespace = main_module.attr("__dict__");
        handle<> ignore(PyRun_String( cmd.str().c_str(),
                                      Py_file_input,
                                      main_namespace.ptr(),
                                      main_namespace.ptr() ));
        boost::python::list l = extract<boost::python::list>(main_namespace["result"]);
        int nrows=boost::python::len(l), ncols;
        for (int j=0; j<nrows; j++)
        {
            boost::python::list row = extract<boost::python::list>(l[j]);
            ncols=boost::python::len(row);
            if (profile.n_rows==0) profile=arma::zeros(nrows, ncols);

            for (int i=0; i<ncols; i++)
            {
                profile(j,i)=extract<double>(row[i]);
            }
        }

    }
    catch (const error_already_set &)
    {
        PyErr_Print();
    }

    return profile;
}

ReferenceDataLibrary refdatalib;

}
