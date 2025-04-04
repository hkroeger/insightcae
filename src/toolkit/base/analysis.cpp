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
#include "boost/filesystem/operations.hpp"
#include "exception.h"
#include "tools.h"

#include <fstream>
#include <cstdlib>
#include <functional>
#include <memory>

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

defineFactoryTable2(
    Analysis, AnalysisFactories, analyses );

defineStaticFunctionTable2(
    "supplemented input data generators",
    Analysis, SupplementedInputDataFactories, supplementedInputDatas );

defineStaticFunctionTable2(
    "default parameters",
    Analysis, DefaultParameterFactories, defaultParameters );

defineStaticFunctionTable2(
    "categories",
    Analysis, CategoryFunctions, categories );

defineStaticFunctionTable2(
    "supported operating systems",
    Analysis, CompatibleOperatingSystemFunctions, compatibleOperatingSystemFunctions );

defineStaticFunctionTable2(
    "parameter set validators",
    Analysis, ValidatorFunctions, validators );

defineStaticFunctionTable2(
    "parameter proposition generators",
    Analysis, PropositionsForParameterFunctions, propositionsForParameter );

defineStaticFunctionTable2(
    "analysis descriptions",
    Analysis, DescriptionFunctions, descriptions );



void Analysis::resetParameters(
    std::shared_ptr<supplementedInputDataBase> sid )
{
    sp_ = sid;
}


std::string Analysis::category()
{
    return "Uncategorized";
}




OperatingSystemSet
Analysis::compatibleOperatingSystems()
{
    return { WindowsOS, LinuxOS };
}




ParameterSet_ValidatorPtr Analysis::validator()
{
    return ParameterSet_ValidatorPtr();
}





std::unique_ptr<ParameterSet> Analysis::getPropositionsForParameter(
    const std::string &parameterPath,
    const ParameterSet &currentParameterValues )
{
    return nullptr;
}




Analysis::Analysis(
    const std::shared_ptr<supplementedInputDataBase> &sp )
    : sp_(sp)
{}


Analysis::~Analysis()
{}



path Analysis::executionPath() const
{
    return spBase().executionPath();
}




ResultSetPtr Analysis::createResultSet() const
{
    auto desc=Analysis::descriptions()(type());
    auto results=std::make_shared<ResultSet>(
        parameters(), desc.name, "Result Report");
    results->introduction() = desc.description;
    return results;
}



AnalysisDescription Analysis::description()
{
    return {"", ""};
}

const ParameterSet &Analysis::parameters() const
{
    return spBase().parameters();
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





AnalysisWithParameters::AnalysisWithParameters(
    const std::shared_ptr<supplementedInputDataBase>& sp )
  : Analysis(sp)
{}







}
