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
#include "boost/thread.hpp"
#include "boost/foreach.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{
  
ProgressDisplayer::~ProgressDisplayer()
{
}

void TextProgressDisplayer::update(const ProgressState& pi)
{
  double iter=pi.first;
  const ProgressVariableList& pvl=pi.second;
  
  BOOST_FOREACH( const ProgressVariableList::value_type& i, pvl)
  {
    const std::string& name = i.first;
    double value = i.second;

    cout << name << "=" << value << "\t";
  }
  cout << endl;
}

  
defineType(Analysis);

/*  
Analysis::Analysis()
{
}
*/
void Analysis::extendSharedSearchPath(const std::string& name)
{
  sharedSearchPath_.push_back(path(name));
}


boost::filesystem::path Analysis::setupExecutionEnvironment()
{
  if (executionPath_()=="")
  {
    executionPath_() = boost::filesystem::unique_path();
  }
  if (!exists(executionPath_()))
    create_directories(executionPath_());
  return executionPath_();
}

void Analysis::setExecutionPath(boost::filesystem::path& exePath)
{
  executionPath_()=exePath;
}

void Analysis::setParameters(const ParameterSet& p)
{
  parameters_.reset(p.clone());
}

path Analysis::executionPath() const
{
  if (executionPath_()=="")
    throw insight::Exception("Temporary analysis storage requested but not yet created!");
  return executionPath_();
}


Analysis::Analysis(const std::string& name, const std::string& description)
: name_(name),
  description_(description),
  executionPath_("Directory to store data files during analysis.\nLeave empty for temporary storage.")
{
}

Analysis::Analysis(const NoParameters&)
: executionPath_("Directory to store data files during analysis.\nLeave empty for temporary storage.")
{
}

void Analysis::setDefaults()
{
  std::string name(type());
  replace_all(name, " ", "_");
  replace_all(name, "/", "-");
  executionPath_()=path(".")/name;
}

Analysis::~Analysis()
{
}

bool Analysis::checkParameters(const ParameterSet& p)
{
  return true;
}

void Analysis::cancel()
{
}


boost::filesystem::path Analysis::getSharedFilePath(const boost::filesystem::path& file)
{
  path userSharedDir( path(getenv("HOME"))/".insight"/"share" );
  path globalSharedDir( path("/usr")/"share"/"insight" );
  
  BOOST_REVERSE_FOREACH( const path& p, sharedSearchPath_)
  {
    if (exists(userSharedDir/p/file)) 
      return userSharedDir/p/file;
    else if (exists(globalSharedDir/p/file)) 
      return globalSharedDir/p/file;
  }
  
  // nothing found
  throw insight::Exception(std::string("Requested shared file ")+file.c_str()+" not found either in global nor user shared directories");
  return path();
}

Analysis* Analysis::clone() const
{
  Analysis *newa=this->lookup(this->type(), NoParameters());
  if (parameters_.get())
    newa->setParameters(*parameters_);
  return newa;
}


defineFactoryTable(Analysis, NoParameters);

class CollectingProgressDisplayer
: public ProgressDisplayer
{
  std::string id_;
  ProgressDisplayer* receiver_;
public:
  CollectingProgressDisplayer(const std::string& id, ProgressDisplayer* receiver)
  : id_(id), receiver_(receiver)
  {}
  
  virtual void update(const ProgressState& pi)
  {
    double maxv=-1e10;
    BOOST_FOREACH( const ProgressVariableList::value_type v, pi.second)
    {
      if (v.second > maxv) maxv=v.second;
    } 
    ProgressVariableList pvl;
    pvl[id_]=maxv;
    receiver_->update(ProgressState(pi.first, pvl));
  }
};

AnalysisWorkerThread::AnalysisWorkerThread(SynchronisedAnalysisQueue* queue, ProgressDisplayer* displayer)
: queue_(queue), displayer_(displayer)
{}

void AnalysisWorkerThread::operator()()
{
  while (!queue_->isEmpty())
  {
    AnalysisInstance ai=queue_->dequeue();
    
    // run analysis and transfer results into given ResultSet object
    CollectingProgressDisplayer pd(boost::get<0>(ai), displayer_);
    boost::get<2>(ai)->transfer( *(*boost::get<1>(ai))(&pd) );
    
    // Make sure we can be interrupted
    boost::this_thread::interruption_point();
  }
}

// Add data to the queue and notify others
void SynchronisedAnalysisQueue::enqueue(const AnalysisInstance& data)
{
  // Acquire lock on the queue
  boost::unique_lock<boost::mutex> lock(m_mutex);
  // Add the data to the queue
  m_queue.push(data);
  // Notify others that data is ready
  m_cond.notify_one();
} // Lock is automatically released here

// Get data from the queue. Wait for data if not available
AnalysisInstance SynchronisedAnalysisQueue::dequeue()
{
  // Acquire lock on the queue
  boost::unique_lock<boost::mutex> lock(m_mutex);

  // When there is no data, wait till someone fills it.
  // Lock is automatically released in the wait and obtained
  // again after the wait
  while (m_queue.size()==0) m_cond.wait(lock);

  // Retrieve the data from the queue
  AnalysisInstance result=m_queue.front();
  processed_.push_back(result);
  m_queue.pop();
  return result;
} // Lock is automatically released here

void SynchronisedAnalysisQueue::cancelAll()
{
  while (!isEmpty())
  {
    boost::get<1>(dequeue())->cancel();
  }
}

    

AnalysisLibraryLoader::AnalysisLibraryLoader()
{
  char *var_usershareddir=getenv("INSIGHT_USERSHAREDDIR");
  char *var_globalshareddir=getenv("INSIGHT_GLOBALSHAREDDIRS");
  
  std::vector<path> paths;
  if (var_usershareddir) paths.push_back(var_usershareddir);
  if (var_globalshareddir) 
  {
    std::vector<string> globals;
    split(globals, var_globalshareddir, is_any_of(" "));
    BOOST_FOREACH(const string& s, globals) paths.push_back(s);
  }
  
  BOOST_FOREACH(const path& p, paths)
  {
    if (is_directory(p))
    {
      path userconfigdir(p);
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
		void *handle = dlopen(location.c_str(), RTLD_NOW|RTLD_GLOBAL);
		if (!handle)
		{
		  std::cout<<"Could not load module library "<<location<<": " << dlerror() << std::endl;
		}
		else
		  handles_.push_back(handle);
	      }
	      
	    }
	  }
	}
      }
    }
  }
}

AnalysisLibraryLoader::~AnalysisLibraryLoader()
{
  BOOST_FOREACH(void *handle, handles_)
  {
    dlclose(handle);
  }
}

AnalysisLibraryLoader AnalysisLibraryLoader::loader;

}