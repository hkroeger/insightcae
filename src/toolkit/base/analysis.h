/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef INSIGHT_ANALYSIS_H
#define INSIGHT_ANALYSIS_H

#include "base/parameterset.h"
#include "base/factory.h"

namespace insight
{

typedef std::map<std::string, double> ProgressVariableList;
typedef std::pair<double, ProgressVariableList> ProgressState;  
  
class ProgressDisplayer
{
public:
  virtual void update(const ProgressState& pi) =0;
};

class Analysis
{
  
public:
  declareFactoryTable(Analysis, NoParameters);
  
protected:
  std::string name_;
  std::string description_;

public:
  //Analysis();
  Analysis(const NoParameters&);
  Analysis(const std::string& name, const std::string& description);
  virtual ~Analysis();
  
  inline const std::string& getName() const { return name_; }
  inline const std::string& getDescription() const { return description_; }

  virtual ParameterSet defaultParameters() const =0;
  virtual bool checkParameters(const ParameterSet& p);
  
  virtual ParameterSet operator()(const ParameterSet& p, ProgressDisplayer* displayer=NULL) =0;
  

};


class AnalysisLibraryLoader
{
public:
  AnalysisLibraryLoader();
};

extern AnalysisLibraryLoader loader;

}

#endif // INSIGHT_ANALYSIS_H
