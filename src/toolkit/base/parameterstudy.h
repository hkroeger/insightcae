/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef INSIGHT_PARAMETERSTUDY_H
#define INSIGHT_PARAMETERSTUDY_H

#include <base/analysis.h>

namespace insight {

typedef std::map<std::string, double> VarParameterList;
typedef std::vector<std::string> RangeParameterList;

class ParameterStudy 
: public Analysis
{
protected:
  void generateInstances
  (
    SynchronisedAnalysisQueue& instances, 
    const ParameterSet& templ, 
    int myIdx, 
    DoubleRangeParameter::RangeList::const_iterator i[]
  );
  
  SynchronisedAnalysisQueue queue_;
  boost::thread_group workers_;
  
  boost::shared_ptr<Analysis> baseAnalysis_;
  RangeParameterList varp_;
  
public:
  declareType("Parameter Study");
  
  ParameterStudy
  (
    const std::string& name, 
    const std::string& description, 
    const Analysis& baseAnalysis, 
    const RangeParameterList& varp
  );
  
  virtual ParameterSet defaultParameters() const;
  
  /**
   * return a table of resutls in case the study runs only over a single parameter
   */
  virtual ResultElementPtr table
  (
    std::string shortDescription,
    std::string longDescription,
    const std::string& varp,
    const std::vector<std::string>& res,
    const std::vector<std::string>* headers = NULL
  ) const;
  
  virtual ResultSetPtr operator()(ProgressDisplayer* displayer = 0);
  virtual void cancel();
};

}

#endif // INSIGHT_PARAMETERSTUDY_H
