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

#include "base/progressdisplayer/prefixedprogressdisplayer.h"

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





// ====================================================================================
// ======== AnaylsisThread

void AnalysisThread::launch(std::function<void(void)> action)
{
  thread_ = boost::thread(

        [this,action](WarningDispatcher* globalWarning)
        {
          try
          {
            warnings.setSuperDispatcher(globalWarning);

            action();

          }
          catch (...)
          {
            auto e = std::current_exception();
            if (exceptionHandler_)
              exceptionHandler_(e);
            else
              exception_ = e;
          }
        },

        &warnings
  );
}

AnalysisThread::AnalysisThread(
    AnalysisPtr analysis,
    ProgressDisplayer *pd,
    std::function<void(void)> preAction,
    std::function<void(void)> postAction,
    std::function<void(std::exception_ptr)> exHdlr
)
  : exceptionHandler_(exHdlr)
{
  launch(
        [this,analysis,pd,preAction,postAction]()
        {
          preAction();
          results_ = (*analysis)( *pd );
          postAction();
        }
  );
}

AnalysisThread::AnalysisThread
(
    std::function<void(void)> action,
    std::function<void(std::exception_ptr)> exHdlr
)
  : exceptionHandler_(exHdlr)
{
  launch(
        [action]()
        {
          action();
        }
  );
}

void AnalysisThread::interrupt()
{
  thread_.interrupt();
}

ResultSetPtr AnalysisThread::join()
{
  thread_.join();
  if (exception_) std::rethrow_exception(exception_);
  return results_;
}











AnalysisWorkerThread::AnalysisWorkerThread ( SynchronisedAnalysisQueue* queue, ProgressDisplayer* displayer )
    :
      displayer_(displayer),
      queue_(queue),
      mainThreadWarningDispatcher_(&warnings)
{}




void AnalysisWorkerThread::operator() ()
{
  warnings.setSuperDispatcher(mainThreadWarningDispatcher_);

  try
  {
    while ( !queue_->isEmpty() )
    {
      AnalysisInstance ai = queue_->dequeue();

      // run analysis and transfer results into given ResultSet object
      PrefixedProgressDisplayer pd(displayer_, ai.name,
                                   PrefixedProgressDisplayer::Prefixed,
                                   PrefixedProgressDisplayer::ParallelPrefix);

      try
      {
        auto& analysis= *(ai.analysis);
        ai.results->transfer( *analysis(pd) ); // call operator() from analysis object
      }
      catch ( const std::exception& e )
      {
        ai.exception = std::current_exception();
        warnings.issue(
              "An exception has occurred while processing the instance "+ai.name+" of the parameter study."
              "The analsis of this instance was not completed.\n"
              "Reason: "+e.what()
              );
      }

      // Make sure we can be interrupted at least between analyses
      boost::this_thread::interruption_point();
    }
  }
  catch ( const std::exception& e )
  {
    exception_=std::current_exception();
  }
}


void AnalysisWorkerThread::rethrowIfNeeded() const
{
  if (exception_) std::rethrow_exception(exception_);
}



// Add data to the queue and notify others
void SynchronisedAnalysisQueue::enqueue ( const AnalysisInstance& data )
{
    boost::unique_lock<boost::mutex> lock ( m_mutex );

    // Add the data to the queue
    m_queue.push ( data );
    // Notify others that data is ready
    m_cond.notify_one();
}


// Get data from the queue. Wait for data if not available
AnalysisInstance SynchronisedAnalysisQueue::dequeue()
{
    boost::unique_lock<boost::mutex> lock ( m_mutex );

    // When there is no data, wait till someone fills it.
    // Lock is automatically released in the wait and obtained
    // again after the wait
    while ( m_queue.size() ==0 )
    {
        m_cond.wait ( lock );
    }

    // Retrieve the data from the queue
    AnalysisInstance result=m_queue.front();
    processed_.push_back ( result );
    m_queue.pop();

    return result;
}


void SynchronisedAnalysisQueue::cancelAll()
{
#warning MISSING!
//    while ( !isEmpty() ) {
//        boost::get<1> ( dequeue() )->cancel();
//    }
}





// ====================================================================================
// ======== AnalysisLibraryLoader


AnalysisLibraryLoader::AnalysisLibraryLoader()
{

    SharedPathList paths;
    for ( const path& p: paths )
    {
        if ( exists(p) && is_directory ( p ) )
        {
            path userconfigdir ( p );
            userconfigdir /= "modules.d";

            if ( exists(userconfigdir) )
            {
              if ( is_directory ( userconfigdir ) )
              {
                for (
                     directory_iterator itr ( userconfigdir );
                     itr != directory_iterator(); ++itr )
                {
                    if ( is_regular_file ( itr->status() ) )
                    {
                        if ( itr->path().extension() == ".module" )
                        {
                            std::ifstream f ( itr->path().string() );
                            std::string type;
                            path location;
                            f>>type>>location;

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
//    for ( void *handle: handles_ ) {
//        //dlclose(handle);
//    }
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
    void *handle = dlopen ( location.c_str(), RTLD_LAZY|RTLD_GLOBAL|RTLD_NODELETE );
    if ( !handle ) 
    {
        std::cerr<<"Could not load module library "<<location<<"!\nReason: " << dlerror() << std::endl;
    } else 
    {
        handles_.push_back ( handle );
    }
#endif
}


AnalysisLibraryLoader loader;





}
