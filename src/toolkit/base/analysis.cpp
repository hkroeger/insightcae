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


#include "analysis.h"
#include "exception.h"

#include <fstream>
#include <cstdlib>
#include <dlfcn.h>

#include "base/boost_include.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{
  
ProgressDisplayer::~ProgressDisplayer()
{
}

bool ProgressDisplayer::stopRun() const
{
  return false;
}

CombinedProgressDisplayer::CombinedProgressDisplayer(CombinedProgressDisplayer::Ops op)
: op_(op)
{}


void CombinedProgressDisplayer::add(ProgressDisplayer* d)
{
  displayers_.push_back(d);
}

void CombinedProgressDisplayer::update(const ProgressState& pi)
{
  int j=0;
  BOOST_FOREACH(ProgressDisplayer* d, displayers_)
  {
//     std::cout<<"exec #"<<(j++)<<std::endl;
    d->update(pi);
  }
}


bool CombinedProgressDisplayer::stopRun() const
{
  bool stop=false;
  
  if ( (op_==AND)&&(displayers_.size()>0) )
    stop=true;
  
  BOOST_FOREACH(const ProgressDisplayer* d, displayers_)
  {
    if (op_==AND)
      stop = stop && d->stopRun();
    else if (op_==OR)
      stop = stop || d->stopRun();
  }
  return stop;
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

ConvergenceAnalysisDisplayer::ConvergenceAnalysisDisplayer(const std::string& progvar, double threshold)
: progvar_(progvar),
  istart_(10),
  co_(15),
  threshold_(threshold),
  converged_(false)
{}

void ConvergenceAnalysisDisplayer::update(const ProgressState& pi)
{
  decltype(pi.second)::const_iterator pv=pi.second.find(progvar_);
  
  if ( pv != pi.second.end() )
  {
//     std::cout << progvar_ << "=" << pv->second <<std::endl;
    trackedValues_.push_back(pv->second);
  }
  
  if (trackedValues_.size() > istart_)
  {
    std::vector<double> ym;
    for (size_t i=istart_; i<trackedValues_.size(); i++)
    {
      size_t i0=i/2;
      double sum=0.0;
      for (size_t j=i0; j<i; j++)
        sum+=trackedValues_[j];
      ym.push_back(sum/double(i-i0));
    }
    
    if (ym.size()>co_)
    {
      double maxrely=0.0;
      for (size_t j=ym.size()-1; j>=ym.size()-co_; j--)
      {
        double rely=fabs(ym[j]-ym[j-1]) / (fabs(ym[j])+1e-10);
        maxrely=std::max(rely, maxrely);
      }
      
      std::cout<<"max rel. change of "<<progvar_<<" = "<<maxrely;
      
      if (maxrely<threshold_)
      {
        std::cout<<" >>> CONVERGED"<<std::endl;
        converged_=true;
      }
      else
      {
        std::cout<<", not converged"<<std::endl;
      }
    }
  }
}

bool ConvergenceAnalysisDisplayer::stopRun() const
{
  return converged_;
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
  
  stepcachefile_=executionPath()/(safe_name()+".stepcache"); // set name, but...
  performedsteps_=AnalysisStepList(); // reset list, don't support restart per default
  
  return executionPath_();
}

void Analysis::setExecutionPath(const boost::filesystem::path& exePath)
{
  executionPath_()=exePath;
}

void Analysis::setParameters(const ParameterSet& p)
{
  parameters_.reset(p.cloneParameterSet());
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
//   std::string name(type());
//   replace_all(name, " ", "_");
//   replace_all(name, "/", "-");
//   executionPath_()=path(".")/name;
  executionPath_()=path(".");
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
  return sharedSearchPath_.getSharedFilePath(file);
}

Analysis* Analysis::clone() const
{
  Analysis *newa=this->lookup(this->type(), NoParameters());
  if (parameters_.get())
    newa->setParameters(*parameters_);
  return newa;
}


defineFactoryTable(Analysis, NoParameters);

CollectingProgressDisplayer::CollectingProgressDisplayer(const std::string& id, ProgressDisplayer* receiver)
: id_(id), receiver_(receiver)
{}

void CollectingProgressDisplayer::update(const ProgressState& pi)
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


AnalysisWorkerThread::AnalysisWorkerThread(SynchronisedAnalysisQueue* queue, ProgressDisplayer* displayer)
: queue_(queue), displayer_(displayer)
{}

void AnalysisWorkerThread::operator()()
{
  try
  {
   while (!queue_->isEmpty())
   {
    AnalysisInstance ai=queue_->dequeue();
    
    // run analysis and transfer results into given ResultSet object
    CollectingProgressDisplayer pd(boost::get<0>(ai), displayer_);
    boost::get<2>(ai)->transfer( *(*boost::get<1>(ai))(&pd) );  // call operator() from analysis object
    
    // Make sure we can be interrupted
    boost::this_thread::interruption_point();
   }
  }
  catch (insight::Exception e)
  {
   std::cerr<<"Exception occurred:"<<std::endl<<e<<std::endl;
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

  SharedPathList paths;
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
	      //cout<<itr->path()<<": type="<<type<<" location="<<location<<endl;
	      
	      if (type=="library")
	      {
		void *handle = dlopen(location.c_str(), RTLD_NOW|RTLD_GLOBAL /*RTLD_LAZY|RTLD_NODELETE*/);
		if (!handle)
		{
		  std::cout<<"Omitted "<<itr->path()<<": Could not load module library "<<location<<": " << dlerror() << std::endl;
		}
		else
		  handles_.push_back(handle);
	      }
	      
	    }
	  }
	}
      }
      
      path pydir(p);
      pydir /= "python_modules";
      if (is_directory(pydir))
      {
	directory_iterator end_itr; // default construction yields past-the-end
	for ( directory_iterator itr( userconfigdir );
	      itr != end_itr;
	      ++itr )
	{
	  if ( is_regular_file(itr->status()) )
	  {
	    if (itr->path().extension() == ".py" )
	    {
// 	      std::ifstream f(itr->path().c_str());
	    }
	  }
	}
      }
	
    }
    else
    {
      //cout<<"Not existing: "<<p<<endl;
    }
  }
}

AnalysisLibraryLoader::~AnalysisLibraryLoader()
{
  BOOST_FOREACH(void *handle, handles_)
  {
    //dlclose(handle);
  }
}

AnalysisLibraryLoader loader;

}
