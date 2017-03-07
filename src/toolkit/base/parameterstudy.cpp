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

// defineType(ParameterStudy);

template<
  class BaseAnalysis,
  const RangeParameterList& var_params
  >
ParameterStudy<BaseAnalysis,var_params>::ParameterStudy
(
  const std::string& name,
  const std::string& description,
  const ParameterSet& ps,
  const boost::filesystem::path& exePath
)
  : Analysis ( name, description, ps, exePath )
{}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
ParameterSet ParameterStudy<BaseAnalysis,var_params>::defaultParameters()
{

  ParameterSet dfp = Analysis::defaultParameters(BaseAnalysis::typeName);
  
  BOOST_FOREACH( const std::string& parname, var_params )
  {
    double orgval=dfp.getDouble(parname);
    dfp.replace( parname, new DoubleRangeParameter(orgval, 0, 1, dfp.get<DoubleParameter>(parname).description().simpleLatex()) );
  }
  
  std::string subname("run");
  if (!dfp.contains(subname))
  {
      dfp.insert(subname, new SubsetParameter("run parameters"));
  }
  
  dfp.getSubset(subname).insert
  (
    "numthread", 
    std::auto_ptr<IntParameter>
    (
      new IntParameter(4, "Maximum number of parallel threads to run at the same time")
    ) 
  );
        
  return dfp;
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void ParameterStudy<BaseAnalysis,var_params>::modifyInstanceParameters(const std::string& subcase_name, ParameterSetPtr& newp) const
{
  // reserved for derived classes
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void ParameterStudy<BaseAnalysis,var_params>::generateInstances
(
  SynchronisedAnalysisQueue& instances, 
  const ParameterSet& templ, 
  int myIdx, 
  DoubleRangeParameter::RangeList::const_iterator i[]
)
{
  if (myIdx == var_params.size())
  {
    // Put it all together
    std::ostringstream n; n<<"subcase";
    ParameterSetPtr newp( templ.cloneParameterSet() /*Analysis::defaultParameters(BaseAnalysis::typeName).cloneParameterSet()*/ );
    for (int j=0; j<var_params.size(); j++)
    {
      // Replace RangeParameter by actual single value
      const DoubleRangeParameter& rp = templ.get<DoubleRangeParameter>(var_params[j]);
      DoubleParameter* p=rp.toDoubleParameter(i[j]);
      newp->replace(var_params[j], p);
      
      std::string nmod(var_params[j]);
      boost::replace_all(nmod, "/", "_");
      n<<"__"<<nmod<<"="<<(*p)();
    }
    
    //newp->getPath("run/dir") /= n.str();
    
    //append instance
    ResultSetPtr emptyresset( new ResultSet(*newp, name_, "Computation instance "+n.str()) );
    modifyInstanceParameters(n.str(), newp);
    path ep=executionPath()/n.str();
//     newinst->setExecutionPath( ep );
    AnalysisPtr newinst( /*baseAnalysis_->clone()*/ Analysis::lookup(BaseAnalysis::typeName, *newp, ep) );
//     newinst->setParameters(*newp);
    instances.enqueue( AnalysisInstance( n.str(), newinst, emptyresset ));

  }
  else
  {
    const DoubleRangeParameter& crp = templ.get<DoubleRangeParameter>(var_params[myIdx]);
    for (i[myIdx] = crp.values().begin(); i[myIdx] != crp.values().end(); ++i[myIdx])
    {
      generateInstances(instances, templ, myIdx+1, i);
    }
  }
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void ParameterStudy<BaseAnalysis,var_params>::cancel()
{
  workers_.interrupt_all();
  queue_.cancelAll();
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
ResultElementPtr ParameterStudy<BaseAnalysis,var_params>::table
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




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void ParameterStudy<BaseAnalysis,var_params>::setupQueue()
{
  DoubleRangeParameter::RangeList::const_iterator iters[var_params.size()];
  
  queue_.clear();
  generateInstances(queue_, parameters(), 0, iters);
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void ParameterStudy<BaseAnalysis,var_params>::processQueue(insight::ProgressDisplayer* displayer)
{
  int nt = std::min( parameters().getInt("run/numthread"), queue_.n_instances() );
  
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




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
insight::ResultSetPtr ParameterStudy<BaseAnalysis,var_params>::evaluateRuns()
{  
  ResultSetPtr results(new ResultSet(parameters(), name_, "Result Summary"));
  
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




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
insight::ResultSetPtr ParameterStudy<BaseAnalysis,var_params>::operator()(insight::ProgressDisplayer* displayer)
{  
  path dir = setupExecutionEnvironment();
  //parameters().saveToFile(dir/"parameters.ist", type());

  setupQueue();
  processQueue(displayer);
  return evaluateRuns();
}




}
