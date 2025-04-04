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
    
  static std::unique_ptr<ParameterSet> defaultParameters(
      const boost::filesystem::path& scriptfile );

  static std::string category(
      const boost::filesystem::path& scriptfile );

  typedef supplementedInputDataBase supplementedInputData;

public:
    PythonAnalysis(
        const boost::filesystem::path& scriptfile,
        const std::shared_ptr<supplementedInputDataBase>& sid );


    virtual ResultSetPtr operator() ( ProgressDisplayer& displayer = consoleProgressDisplayer );

    static std::string category() { return "Python Analysis"; }

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
