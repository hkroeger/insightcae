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


#ifndef INSIGHT_PYTHONANALYSIS_H
#define INSIGHT_PYTHONANALYSIS_H

#include "base/analysis.h"


namespace insight
{

class PythonAnalysis
: public Analysis
{
    const boost::filesystem::path& scriptfile_;
    

public:
    
  class PythonAnalysisFactory
    : public Analysis::Factory
  {
    const boost::filesystem::path scriptfile_;
    
  public:
    boost::function<ParameterSet(void)> defaultParametersWrapper_;
    boost::function<std::string(void)> categoryWrapper_;
    
    PythonAnalysisFactory ( const boost::filesystem::path& scriptfile );
    virtual ~PythonAnalysisFactory();
    
    virtual Analysis* operator() 
    (
      const ParameterSet& ps,
      const boost::filesystem::path& exePath
    ) const;
  
    ParameterSet defaultParameters() const;
    
    std::string category() const;
  };
  typedef boost::shared_ptr<PythonAnalysisFactory> PythonAnalysisFactoryPtr;
  
  static std::set<PythonAnalysisFactoryPtr> pythonAnalysisFactories_;

public:
    PythonAnalysis(const boost::filesystem::path& scriptfile, const ParameterSet& ps, const boost::filesystem::path& exePath );
    virtual ResultSetPtr operator() ( ProgressDisplayer* displayer=NULL );
};




class PythonAnalysisLoader
{

public:
    PythonAnalysisLoader();
    ~PythonAnalysisLoader();
    
};




extern PythonAnalysisLoader pyloader;




}

#endif // INSIGHT_ANALYSIS_H
