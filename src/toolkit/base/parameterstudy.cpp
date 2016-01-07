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

#include "parameterstudy.h"


#include "base/resultset.h"
#include "parameterstudy.h"
#include "base/plottools.h"

#include "boost/assign.hpp"
#include "boost/ptr_container/ptr_deque.hpp"
#include "boost/thread.hpp"
#include "boost/assign/ptr_map_inserter.hpp"

using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{

defineType(ParameterStudy);

ParameterStudy::ParameterStudy
(
  const std::string& name, 
  const std::string& description, 
  const Analysis& baseAnalysis, 
  const RangeParameterList& varp
)
: Analysis(name, description),
  baseAnalysis_(baseAnalysis.clone()),
  varp_(varp)
{}

void ParameterStudy::setRangeParameters(const RangeParameterList& varp)
{
  varp_=varp;
}


ParameterSet ParameterStudy::defaultParameters() const
{

  ParameterSet dfp = baseAnalysis_->defaultParameters();
  
  BOOST_FOREACH( const std::string& parname, varp_ )
  {
    double orgval=dfp.getDouble(parname);
    dfp.replace( parname, new DoubleRangeParameter(orgval, 0, 1, dfp.get<DoubleParameter>(parname).description()) );
  }
  
  dfp.getSubset("run").insert
  (
    "numthread", 
    std::auto_ptr<IntParameter>
    (
      new IntParameter(4, "Maximum number of parallel threads to run at the same time")
    ) 
  );
        
  return dfp;
}

void ParameterStudy::modifyInstanceParameters(const std::string& subcase_name, ParameterSetPtr& newp) const
{
  // reserved for derived classes
}

void ParameterStudy::generateInstances
(
  SynchronisedAnalysisQueue& instances, 
  const ParameterSet& templ, 
  int myIdx, 
  DoubleRangeParameter::RangeList::const_iterator i[]
)
{
  if (myIdx == varp_.size())
  {
    // Put it all together
    std::ostringstream n; n<<"subcase";
    ParameterSetPtr newp(templ.cloneParameterSet());
    for (int j=0; j<varp_.size(); j++)
    {
      // Replace RangeParameter by actual single value
      const DoubleRangeParameter& rp = templ.get<DoubleRangeParameter>(varp_[j]);
      DoubleParameter* p=rp.toDoubleParameter(i[j]);
      newp->replace(varp_[j], p);
      
      std::string nmod(varp_[j]);
      boost::replace_all(nmod, "/", "_");
      n<<"__"<<nmod<<"="<<(*p)();
    }
    
    //newp->getPath("run/dir") /= n.str();
    
    //append instance
    ResultSetPtr emptyresset( new ResultSet(*newp, name_, "Computation instance "+n.str()) );
    AnalysisPtr newinst(baseAnalysis_->clone());
    modifyInstanceParameters(n.str(), newp);
    newinst->setParameters(*newp);
    path ep=executionPath()/n.str();
    newinst->setExecutionPath( ep );
    instances.enqueue( AnalysisInstance( n.str(), newinst, emptyresset ));

  }
  else
  {
    const DoubleRangeParameter& crp = templ.get<DoubleRangeParameter>(varp_[myIdx]);
    for (i[myIdx] = crp.values().begin(); i[myIdx] != crp.values().end(); ++i[myIdx])
    {
      generateInstances(instances, templ, myIdx+1, i);
    }
  }
}

void ParameterStudy::cancel()
{
  workers_.interrupt_all();
  queue_.cancelAll();
}

ResultElementPtr ParameterStudy::table
(
  std::string shortDescription,
  std::string longDescription,
  const std::string& varp,
  const std::vector<std::string>& res,
  const std::vector<std::string>* headers
) const
{
  TabularResult::Table tab;
  BOOST_FOREACH( const AnalysisInstance& ai, queue_.processed() )
  {
    const AnalysisPtr& a = get<1>(ai);
    const ResultSetPtr& r = get<2>(ai);

    double x=a->parameters().getDouble(varp);
    
    std::vector<double> row;
    row.push_back(x);
    BOOST_FOREACH( const std::string& ren, res)
    {
      row.push_back( dynamic_cast<ScalarResult*>(r->at(ren).get())->value() ); 
    }
    tab.push_back( row );    
  }
  
  std::vector<std::string> heads;
  if (headers) 
    heads=*headers;
  else
  {
    heads=res;
    heads.insert(heads.begin(), varp);
  }
  
  return ResultElementPtr
  (
    new TabularResult(heads, tab, shortDescription, longDescription, "")
  );
  
}

void ParameterStudy::setupQueue()
{
  const ParameterSet& p = *parameters_;
  
  DoubleRangeParameter::RangeList::const_iterator iters[varp_.size()];
  
  queue_.clear();
  generateInstances(queue_, p, 0, iters);
}

void ParameterStudy::processQueue(insight::ProgressDisplayer* displayer)
{
  const ParameterSet& p = *parameters_;
  int nt = std::min( p.getInt("run/numthread"), queue_.n_instances() );
  
  boost::ptr_vector<AnalysisWorkerThread> threads;
  for (int i=0; i<nt; i++)
  {
    threads.push_back(new AnalysisWorkerThread(&queue_, displayer));
  }
  
  BOOST_FOREACH(AnalysisWorkerThread& t, threads)
  {
    workers_.create_thread(boost::ref(t));
  }
  
  //wait for computation to finish
  workers_.join_all();
}

insight::ResultSetPtr ParameterStudy::evaluateRuns()
{  
  const ParameterSet& p = *parameters_;

  ResultSetPtr results(new ResultSet(p, name_, "Result Summary"));
  
  TabularResult::Table force_data;
  BOOST_FOREACH( const AnalysisInstance& ai, queue_.processed() )
  {
    const std::string& n = get<0>(ai);
    const AnalysisPtr& a = get<1>(ai);
    const ResultSetPtr& r = get<2>(ai);

    std::string key=n/*+", "+r->title()+" ("+r->subtitle()+")"*/;
    results->insert( key, r->clone() );    
  }
  
  return results;
}

insight::ResultSetPtr ParameterStudy::operator()(insight::ProgressDisplayer* displayer)
{  
  const ParameterSet& p = *parameters_;

  path dir = setupExecutionEnvironment();
  //p.saveToFile(dir/"parameters.ist", type());

  setupQueue();
  processQueue(displayer);
  return evaluateRuns();
}

}
