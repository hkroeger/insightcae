
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
 
ReferenceDataLibrary::ReferenceDataLibrary()
{
  path dir=path(getenv("HOME"))/"Referenzdaten";
  
  if ( exists( dir ) ) 
  {
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( dir );
          itr != end_itr;
          ++itr )
    {
      if ( is_directory(itr->status()) )
      {
	path mod=itr->path()/"lookup.py";
        if (exists(mod))
	{
	  datasets_[itr->path().filename().string()]=mod;
	  //cout<<"Added "<<itr->path().filename().string()<<": "<<mod<<endl;
	}
      }
    }
  }

  Py_Initialize();

}

ReferenceDataLibrary::~ReferenceDataLibrary()
{
  Py_Finalize();
}
  
arma::mat ReferenceDataLibrary::getProfile(const std::string& dataSetName, const std::string& path) const
{
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
           "import imp;"
           "mod=imp.load_source('mod', "<<i->second<<");"
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