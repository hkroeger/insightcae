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
  
  // == Modify parameter set of base analysis:

  // replace selected double parameters by range of parameters each
  for(const std::string& parname: var_params )
  {
    double orgval=dfp.getDouble(parname);
    dfp.replace
            (
                parname,
                new DoubleRangeParameter(orgval, 0, 1, dfp.get<DoubleParameter>(parname).description().simpleLatex())
            );
  }
  
  // if there is no run/numthread parameter, add one
  std::string subname("run");
  if (!dfp.contains(subname))
  {
      dfp.emplace(subname, make<SubsetParameter>("run parameters"));
  }
  
  dfp.getSubset(subname).emplace
  (
    "numthread", 
    make<IntParameter>
    (
      4, "Maximum number of parallel threads to run at the same time"
    ) 
  );
        
  return dfp;
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void ParameterStudy<BaseAnalysis,var_params>::modifyInstanceParameters(const std::string& , ParameterSetPtr& ) const
{
  // reserved for derived classes
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void ParameterStudy<BaseAnalysis,var_params>::generateInstance
(
  SynchronisedAnalysisQueue& instances,
  const ParameterSet& templ,
  DoubleRangeParameter::RangeList::const_iterator i[]
)
{
    std::ostringstream n; n<<"subcase";

    ParameterSetPtr newp( templ.cloneParameterSet() );
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

    //append instance
    ResultSetPtr emptyresset( new ResultSet(*newp, name_, "Computation instance "+n.str()) );
    modifyInstanceParameters(n.str(), newp);
    path ep=executionPath()/n.str();

    // create analysis object
    AnalysisPtr newinst( Analysis::lookup(BaseAnalysis::typeName, *newp, ep) );
    newinst->setKeepExecutionDirectory();

    instances.enqueue( AnalysisInstance{ n.str(), newinst, emptyresset, nullptr } );
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
      generateInstance(instances, templ, i);
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




//template<
//  class BaseAnalysis,
//  const RangeParameterList& var_params
//>
//void ParameterStudy<BaseAnalysis,var_params>::cancel()
//{
//  workers_.interrupt_all();
////  queue_.cancelAll();
//}




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
  for( const AnalysisInstance& ai: queue_.processed() )
  {
    double x = ai.analysis->parameters().getDouble(varp);
    
    std::vector<double> row;
    row.push_back(x);
    for( const std::string& ren: res)
    {
      row.push_back( dynamic_cast<ScalarResult*>(ai.results->at(ren).get())->value() );
    }
    tab.push_back( row );    
  }
  
  std::vector<std::string> heads;
  if (headers) 
  {
    heads=*headers;
  }
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
void ParameterStudy<BaseAnalysis,var_params>::processQueue(insight::ProgressDisplayer& displayer)
{
  int nt = std::min( parameters().getInt("run/numthread"), int(queue_.n_instances()) );
  
  boost::ptr_vector<AnalysisWorkerThread> threads;
  for (int i=0; i<nt; i++)
  {
    threads.push_back(new AnalysisWorkerThread(&queue_, &displayer));
  }
  
  for(auto& t: threads)
  {
    workers_.create_thread(boost::ref(t));
  }
  
  //wait for computation to finish
  workers_.join_all();

  for(auto& t: threads)
  {
    t.rethrowIfNeeded();
  }

  int nFailed=0;
  for (const AnalysisInstance& ai: queue_.processed())
  {
      if (ai.exception) nFailed++;
  }
  if (queue_.processed().size()-nFailed==0)
  {
      throw insight::Exception("No analsis of any variant has been successful!");
  }
}


struct ai_sort_pred {
    bool operator()(const AnalysisInstance& ai1, const AnalysisInstance& ai2) {
         return ( ai1.name < ai2.name );
      }
};

template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
insight::ResultSetPtr ParameterStudy<BaseAnalysis,var_params>::evaluateRuns()
{  
  ResultSetPtr results(new ResultSet(parameters(), name_, "Result Summary"));
  
  TabularResult::Table force_data;
  AnalysisInstanceList processed_analyses = queue_.processed();
  sort(
          processed_analyses.begin(),
          processed_analyses.end(),
          ai_sort_pred()
      );

  Ordering o(1000);
  for( const AnalysisInstance& ai:  processed_analyses)
  {
      if (!ai.exception)
      {
          results->insert( ai.name, ai.results->clone() ).setOrder(o.next());
      }
      else
      {
          insight::Warning("Analysis of the variant "+ai.name+" failed and the results are not considered in the report.");
      }
  }
  
  return results;
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
insight::ResultSetPtr ParameterStudy<BaseAnalysis,var_params>::operator()(insight::ProgressDisplayer& displayer)
{  
  path dir = setupExecutionEnvironment();

  setupQueue();

  processQueue(displayer);

  return evaluateRuns();
}




}
