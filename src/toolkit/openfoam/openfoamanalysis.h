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
: public Analysis
{
public:
  
#include "openfoamanalysis__OpenFOAMAnalysis__Parameters.h"
/*
PARAMETERSET>>> OpenFOAMAnalysis Parameters

run = set 
{	
 machine 	= 	string 	"" 	"machine or queue, where the external commands are executed on"
 OFEname 	= 	string 	"OF23x" "identifier of the OpenFOAM installation, that shall be used"
 np 		= 	int 	1 	"number of processors for parallel run (less or equal 1 means serial execution)"
 deltaT 	= 	double 	1.0 	"simulation time step"
 endTime 	= 	double 	1000.0 	"simulation time at which the solver should stop"
 mapFrom 	= 	path 	"" 	"Map solution from specified case, potentialinit is skipped when specified"
 potentialinit 	= 	bool 	false 	"Whether to initialize the flow field by potentialFoam when no mapping is done"
 evaluateonly	= 	bool 	false 	"Whether to skip solver run and do only the evaluation"
} "Execution parameters"

mesh = set
{
 linkmesh 	= 	path 	"" 	"path to another case, from what the mesh shall be linked"
} "Properties of the computational mesh"

fluid = set
{
 turbulenceModel = 	selection 
 ( 
  kOmegaSST 
 ) kOmegaSST 				"Turbulence model"
} "Parameters of the fluid"

eval = set
{
 reportdicts 	= 	bool 	true 	"Include dictionaries into report"
} "Parameters for evaluation after solver run"

<<<PARAMETERSET
*/

protected:
    bool stopFlag_;
    ResultSetPtr derivedInputData_;

public:
    OpenFOAMAnalysis(const std::string& name, const std::string& description);
    
    virtual void cancel();
    
    virtual insight::ParameterSet defaultParameters() const;
    virtual boost::filesystem::path setupExecutionEnvironment();
    
    virtual void reportIntermediateParameter(const std::string& name, double value, const std::string& description="", const std::string& unit="");
    
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
    
    virtual void mapFromOther(OpenFOAMCase& cm, const boost::filesystem::path& mapFromPath, bool is_parallel);
    virtual void initializeSolverRun(OpenFOAMCase& cm);
    virtual void runSolver(ProgressDisplayer* displayer, OpenFOAMCase& cm);
    virtual void finalizeSolverRun(OpenFOAMCase& cm);
    
    virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm);
    
    /**
     * integrate all steps before the actual run
     */
    virtual void createCaseOnDisk(OpenFOAMCase& cm);
    
    virtual ResultSetPtr operator()(ProgressDisplayer* displayer=NULL);
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
      const RangeParameterList& varp = RangeParameterList(),
      bool subcasesRemesh=false
    );

    virtual void modifyInstanceParameters(const std::string& subcase_name, ParameterSetPtr& newp) const;
    virtual ResultSetPtr operator()(ProgressDisplayer* displayer = 0);

    virtual void evaluateCombinedResults(ResultSetPtr& results);
};

}

#endif // INSIGHT_OPENFOAMANALYSIS_H
