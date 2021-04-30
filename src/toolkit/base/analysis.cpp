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
#include "tools.h"

#include <fstream>
#include <cstdlib>
#include <dlfcn.h>

#include "base/boost_include.h"
#include "boost/function.hpp"
#include "boost/thread.hpp"

#ifdef WIN32
#include "libloaderapi.h"
#endif

using namespace std;
using namespace boost;
using namespace boost::filesystem;


namespace insight
{
  
  
defineType ( Analysis );

defineFactoryTable 
( 
  Analysis,
  LIST(
      const ParameterSet& ps,
      const boost::filesystem::path& exePath
  ),
  LIST(ps, exePath)
);

defineStaticFunctionTable(Analysis, defaultParameters, ParameterSet);
defineStaticFunctionTable(Analysis, category, std::string);
defineStaticFunctionTable(Analysis, validator, ParameterSet_ValidatorPtr);
defineStaticFunctionTable(Analysis, visualizer, ParameterSet_VisualizerPtr);


std::string Analysis::category()
{
    return "Uncategorized";
}

ParameterSet_ValidatorPtr Analysis::validator()
{
    return ParameterSet_ValidatorPtr();
}

ParameterSet_VisualizerPtr Analysis::visualizer()
{
    return ParameterSet_VisualizerPtr();
}


void Analysis::extendSharedSearchPath ( const std::string& name )
{
    sharedSearchPath_.push_back ( path ( name ) );
}


boost::filesystem::path Analysis::setupExecutionEnvironment()
{
  if ( executionPath_ =="" )
    {
      executionPath_ =
          boost::filesystem::unique_path(
            timeCodePrefix()+"_analysis-%%%%%%"
            );
      if (!enforceExecutionPathRemovalBehaviour_) removeExecutionPath_=true;
    }

  if ( !exists ( executionPath_ ) )
    {
      create_directories ( executionPath_ );
      if (!enforceExecutionPathRemovalBehaviour_) removeExecutionPath_=true;
    }

  return executionPath_;
}


void Analysis::setExecutionPath ( const boost::filesystem::path& exePath )
{
    executionPath_ =exePath;
}

void Analysis::setParameters ( const ParameterSet& p )
{
    parameters_ = p;
}

path Analysis::executionPath() const
{
    if ( executionPath_ =="" ) {
        throw insight::Exception ( "Temporary analysis storage requested but not yet created!" );
    }
    return executionPath_;
}

path Analysis::createExecutionPathIfNonexistent()
{
  if ( executionPath_ =="" )
  {
    setupExecutionEnvironment();
  }
  return executionPath();
}

Analysis::Analysis ( const std::string& name, const std::string& description, const ParameterSet& ps, const boost::filesystem::path& exePath )
: name_ ( name ),
  description_ ( description ),
  executionPath_ ( exePath ),
  removeExecutionPath_(false),
  enforceExecutionPathRemovalBehaviour_(false)
{
  setParameters(ps);
  setExecutionPath(exePath);
}


// void Analysis::setDefaults()
// {
// //   std::string name(type());
// //   replace_all(name, " ", "_");
// //   replace_all(name, "/", "-");
// //   executionPath_()=path(".")/name;
//     executionPath_() =path ( "." );
// }

Analysis::~Analysis()
{
    if (removeExecutionPath_)
    {
        remove_all(executionPath_);
    }
}


bool Analysis::checkParameters() const
{
    return true;
}


boost::filesystem::path Analysis::getSharedFilePath ( const boost::filesystem::path& file )
{
    return sharedSearchPath_.getSharedFilePath ( file );
}

Analysis* Analysis::clone() const
{
    return this->lookup ( this->type(), parameters_, executionPath_ );
}





CollectingProgressDisplayer::CollectingProgressDisplayer ( const std::string& id, ProgressDisplayer* receiver )
    : id_ ( id ), receiver_ ( receiver )
{}

void CollectingProgressDisplayer::update ( const ProgressState& pi )
{
//    double maxv=-1e10;
//    for ( const ProgressVariableList::value_type v: pi.second ) {
//        if ( v.second > maxv ) {
//            maxv=v.second;
//        }
//    }
//    ProgressVariableList pvl;
//    pvl[id_]=maxv;
//    receiver_->update ( ProgressState ( pi.first, pvl ) );

  ProgressVariableList pvl;
  for (const ProgressVariableList::value_type& v: pi.second )
  {
    pvl[id_ + "/" + v.first]=v.second;
  }
  receiver_->update ( ProgressState(pi.first, pvl) );
}




AnalysisWorkerThread::AnalysisWorkerThread ( SynchronisedAnalysisQueue* queue, ProgressDisplayer* displayer )
    :
      displayer_ ( displayer ),
      queue_ ( queue )
{}

void AnalysisWorkerThread::operator() ()
{
  try
  {
    while ( !queue_->isEmpty() ) {
      AnalysisInstance ai=queue_->dequeue();

      // run analysis and transfer results into given ResultSet object
      CollectingProgressDisplayer pd ( boost::get<0> ( ai ), displayer_ );
      boost::get<2> ( ai )->transfer ( * ( *boost::get<1> ( ai ) ) ( pd ) ); // call operator() from analysis object

      // Make sure we can be interrupted
      boost::this_thread::interruption_point();
    }
  }
  catch ( const std::exception& e )
  {
    insight::Warning(std::string("Exception occurred:\n")+e.what());
  }
}




// Add data to the queue and notify others
void SynchronisedAnalysisQueue::enqueue ( const AnalysisInstance& data )
{
    // Acquire lock on the queue
    boost::unique_lock<boost::mutex> lock ( m_mutex );
    // Add the data to the queue
    m_queue.push ( data );
    // Notify others that data is ready
    m_cond.notify_one();
} // Lock is automatically released here


// Get data from the queue. Wait for data if not available
AnalysisInstance SynchronisedAnalysisQueue::dequeue()
{
    // Acquire lock on the queue
    boost::unique_lock<boost::mutex> lock ( m_mutex );

    // When there is no data, wait till someone fills it.
    // Lock is automatically released in the wait and obtained
    // again after the wait
    while ( m_queue.size() ==0 ) {
        m_cond.wait ( lock );
    }

    // Retrieve the data from the queue
    AnalysisInstance result=m_queue.front();
    processed_.push_back ( result );
    m_queue.pop();
    return result;
} // Lock is automatically released here


void SynchronisedAnalysisQueue::cancelAll()
{
#warning MISSING!
//    while ( !isEmpty() ) {
//        boost::get<1> ( dequeue() )->cancel();
//    }
}

    

AnalysisLibraryLoader::AnalysisLibraryLoader()
{

    SharedPathList paths;
    for ( const path& p: /*SharedPathList::searchPathList*/paths )
    {
        if ( exists(p) && is_directory ( p ) )
        {
            path userconfigdir ( p );
            userconfigdir /= "modules.d";

            if ( exists(userconfigdir) )
            {
              if ( is_directory ( userconfigdir ) )
              {
                directory_iterator end_itr; // default construction yields past-the-end
                for ( directory_iterator itr ( userconfigdir );
                        itr != end_itr;
                        ++itr )
                {
                    if ( is_regular_file ( itr->status() ) )
                    {
                        if ( itr->path().extension() == ".module" )
                        {
                            std::ifstream f ( itr->path().string() );
                            std::string type;
                            path location;
                            f>>type>>location;
                            //cout<<itr->path()<<": type="<<type<<" location="<<location<<endl;

                            if ( type=="library" )
                            {
                                addLibrary(location);
                            }
                        }
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
    for ( void *handle: handles_ ) {
        //dlclose(handle);
    }
}

void AnalysisLibraryLoader::addLibrary(const boost::filesystem::path& location)
{
#ifdef WIN32
  HMODULE lib = LoadLibraryA(location.string().c_str());
  if (!lib)
  {
    std::cerr<<"Could not load module library "<<location<< std::endl;
  }
#else
    void *handle = dlopen ( location.string().c_str(), RTLD_NOW|RTLD_GLOBAL /*RTLD_LAZY|RTLD_NODELETE*/ );
    if ( !handle ) 
    {
        std::cerr<<"Could not load module library "<<location<<": " << dlerror() << std::endl;
    } else 
    {
//        std::cout<<"Loaded module library "<<location << std::endl;
        handles_.push_back ( handle );
    }
#endif
}


AnalysisLibraryLoader loader;

}
