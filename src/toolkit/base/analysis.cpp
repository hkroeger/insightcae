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

AnalysisWorkerThread::AnalysisWorkerThread(SynchronisedAnalysisQueue* queue, Analysis& analysis, ProgressDisplayer* displayer)
: queue_(queue), analysis_(analysis), displayer_(displayer)
{}

void AnalysisWorkerThread::operator()()
{
  while (!queue_->isEmpty())
  {
    AnalysisInstance ai=queue_->dequeue();
    
    // run analysis and transfer results into given ResultSet object
    CollectingProgressDisplayer pd(get<0>(ai), displayer_);
    get<2>(ai)->transfer( *analysis_(*get<1>(ai), &pd) );
    
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