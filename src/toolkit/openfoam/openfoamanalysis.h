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


#ifndef INSIGHT_OPENFOAMANALYSIS_H
#define INSIGHT_OPENFOAMANALYSIS_H

#include "base/analysis.h"
#include "openfoam/openfoamcase.h"
#include "base/parameterstudy.h"


namespace insight {
  
//class OpenFOAMCase;
class turbulenceModel;
  
turbulenceModel* insertTurbulenceModel(OpenFOAMCase& cm, const std::string& name);

class OpenFOAMAnalysis
: public insight::Analysis
{
protected:
    bool stopFlag_;

public:
    OpenFOAMAnalysis(const std::string& name, const std::string& description);
    
    virtual void cancel();
    
    virtual insight::ParameterSet defaultParameters() const;
    virtual boost::filesystem::path setupExecutionEnvironment();
    
    virtual void calcDerivedInputData();
    virtual void createMesh(OpenFOAMCase& cm) =0;
    virtual void createCase(OpenFOAMCase& cm) =0;
    
    virtual void createDictsInMemory(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
    
    /**
     * Customize dictionaries before they get written to disk
     */
    virtual void applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
    
    virtual void writeDictsToDisk(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
    
    /**
     * Do modifications to the case when it has been created on disk
     */
    virtual void applyCustomPreprocessing(OpenFOAMCase& cm);
    
    virtual void initializeSolverRun(OpenFOAMCase& cm);
    virtual void runSolver(ProgressDisplayer* displayer, OpenFOAMCase& cm);
    virtual void finalizeSolverRun(OpenFOAMCase& cm);
    
    virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm);
    
    virtual ResultSetPtr operator()(ProgressDisplayer* displayer);
};


class OpenFOAMParameterStudy
: public ParameterStudy
{
protected:
  bool subcasesRemesh_;
public:
    declareType("OpenFOAM Parameter Study");
    
    OpenFOAMParameterStudy
    (
      const std::string& name, 
      const std::string& description, 
      const OpenFOAMAnalysis& baseAnalysis, 
      const RangeParameterList& varp,
      bool subcasesRemesh=false
    );

    virtual void modifyInstanceParameters(const std::string& subcase_name, ParameterSetPtr& newp) const;
    virtual ResultSetPtr operator()(ProgressDisplayer* displayer = 0);

    virtual void evaluateCombinedResults(ResultSetPtr& results);
};

}

#endif // INSIGHT_OPENFOAMANALYSIS_H
