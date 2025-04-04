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

#ifndef INSIGHT_PARAMETERSTUDY_H
#define INSIGHT_PARAMETERSTUDY_H

#include <base/analysis.h>
#include "base/parameters/doublerangeparameter.h"




namespace insight {




typedef std::map<std::string, double> VarParameterList;
typedef std::vector<std::string> RangeParameterList;




enum TableInputType { DoubleInputParameter = 0, ScalarResultElement = 1 };




template<
 class BaseAnalysis,
 const RangeParameterList& var_params = RangeParameterList()
 >
class ParameterStudy 
: public Analysis
{

protected:
  void generateInstance
  (
    SynchronisedAnalysisQueue& instances,
    const ParameterSet& templ,
    DoubleRangeParameter::RangeList::const_iterator i[]
  );

  void generateInstances
  (
    SynchronisedAnalysisQueue& instances, 
    const ParameterSet& templ,
    int myIdx, 
    DoubleRangeParameter::RangeList::const_iterator i[]
  );
  
  SynchronisedAnalysisQueue queue_;
  boost::thread_group workers_;

  
public:
//   declareType("Parameter Study");
  typedef supplementedInputDataBase supplementedInputData;
  
  ParameterStudy(
    const std::shared_ptr<supplementedInputDataBase>& sp );

  static std::unique_ptr<ParameterSet> defaultParameters();
  
  /**
   * return a table of results in case the study runs only over a single parameter
   */
  virtual ResultElementPtr table
  (
    std::string shortDescription,
    std::string longDescription,
    const std::string& varp,
    const std::vector<std::string>& res,
    const std::vector<std::string>* headers = nullptr,
    TableInputType tit = DoubleInputParameter
  ) const;
  
  virtual void modifyInstanceParameters(const std::string& subcase_name, ParameterSet& newp) const;
  virtual void setupQueue();
  virtual void processQueue(insight::ProgressDisplayer& displayer);
  virtual ResultSetPtr evaluateRuns();
  
  ResultSetPtr operator()(ProgressDisplayer& displayer = consoleProgressDisplayer) override;

  static std::string category() { return BaseAnalysis::category(); }
  static AnalysisDescription description()
  {
      auto basedesc=BaseAnalysis::description();
      return {"Parameter Study of "+basedesc.name, basedesc.description};
  }
};




}




#include "parameterstudy.cpp"




#endif // INSIGHT_PARAMETERSTUDY_H
