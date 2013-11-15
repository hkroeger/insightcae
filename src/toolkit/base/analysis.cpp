/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "analysis.h"
#include "exception.h"

#include <fstream>
#include <cstdlib>
#include <dlfcn.h>

#include "boost/filesystem.hpp"

using namespace std;
using namespace boost::filesystem;

namespace insight
{
/*  
Analysis::Analysis()
{
}
*/

Analysis::Analysis(const std::string& name, const std::string& description)
: name_(name),
  description_(description)
{
}

Analysis::Analysis(const NoParameters&)
{
}

Analysis::~Analysis()
{
}

bool Analysis::checkParameters(const ParameterSet& p)
{
  return true;
}

defineFactoryTable(Analysis, NoParameters);

AnalysisThread::AnalysisThread(boost::shared_ptr<Analysis> analysis, const ParameterSet& p, ProgressDisplayer* displayer)
: analysis_(analysis), p_(p), displayer_(displayer)
{}

void AnalysisThread::operator()()
{
  result_=(*analysis_)(p_, displayer_);
}


AnalysisLibraryLoader::AnalysisLibraryLoader()
{
  char *homedir=getenv("HOME");
  if (homedir)
  {
    path userconfigdir(homedir);
    userconfigdir /= ".insight";
    userconfigdir /= "modules.d";
    if (is_directory(userconfigdir))
    {
      directory_iterator end_itr; // default construction yields past-the-end
      for ( directory_iterator itr( userconfigdir );
	    itr != end_itr;
	    ++itr )
      {
	if ( is_regular_file(itr->status()) )
	{
	  if (itr->path().extension() == ".module" )
	  {
	    std::ifstream f(itr->path().c_str());
	    std::string type;
	    path location;
	    f>>type>>location;
	    cout<<itr->path()<<": type="<<type<<" location="<<location<<endl;
	    
	    if (type=="library")
	    {
	      void *handle = dlopen(location.c_str(), RTLD_NOW);
	      if (!handle)
	      {
		throw insight::Exception(std::string("Could not load module library: ")+dlerror());
	      }
	    }
	    
	  }
	}
      }
    }
  }
}

AnalysisLibraryLoader loader;

}