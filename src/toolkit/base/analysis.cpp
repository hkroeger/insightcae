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

#include "base/boost_include.h"
#include "boost/function.hpp"
#include "boost/thread.hpp"
#include "boost/process.hpp"



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
      const boost::filesystem::path& exePath,
      ProgressDisplayer& displayer
  ),
  LIST(ps, exePath, displayer)
);

defineStaticFunctionTable(Analysis, defaultParameters, ParameterSet);
defineStaticFunctionTable(Analysis, category, std::string);
defineStaticFunctionTable(Analysis, validator, ParameterSet_ValidatorPtr);
defineStaticFunctionTable(Analysis, visualizer, std::shared_ptr<ParameterSetVisualizer>);


std::string Analysis::category()
{
    return "Uncategorized";
}

ParameterSet_ValidatorPtr Analysis::validator()
{
    return ParameterSet_ValidatorPtr();
}

std::shared_ptr<ParameterSetVisualizer> Analysis::visualizer()
{
    return std::shared_ptr<ParameterSetVisualizer>();
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

//void Analysis::setParameters ( const ParameterSet& p )
//{
//    parameters_ = p;
//}

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

Analysis::Analysis (
    const std::string& name,
    const std::string& description,
    const ParameterSet& /*ps*/,
    const boost::filesystem::path& exePath,
    ProgressDisplayer& /*displayer*/ )
: name_ ( name ),
  description_ ( description ),
  executionPath_ ( exePath ),
  removeExecutionPath_(false),
  enforceExecutionPathRemovalBehaviour_(false)
{
//  setParameters(ps);
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


//bool Analysis::checkParameters() const
//{
//    return true;
//}



//Analysis* Analysis::clone() const
//{
//    return this->lookup ( this->type(), parameters_, executionPath_ );
//}






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




}
