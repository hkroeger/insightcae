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


#include "openfoamanalysis.h"
#include "openfoamtools.h"
#include "openfoamcaseelements.h"

#include "base/boost_include.h"

using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;
using namespace std;

namespace insight
{

turbulenceModel* insertTurbulenceModel(OpenFOAMCase& cm, const std::string& name)
{
  turbulenceModel* model = turbulenceModel::lookup(name, cm);
  
  if (!model) 
    throw insight::Exception("Unrecognized RASModel selection: "+name);
  
  return cm.insert(model);
}


void OpenFOAMAnalysis::cancel()
{
  stopFlag_=true;
}

OpenFOAMAnalysis::OpenFOAMAnalysis(const std::string& name, const std::string& description)
: Analysis(name, description)
{
}

ParameterSet OpenFOAMAnalysis::defaultParameters() const
{
  ParameterSet p(Parameters::makeDefault());
  p.getSubset("fluid").get<SelectionParameter>("turbulenceModel").items()=turbulenceModel::factoryToC();
  return p;
  
//   return ParameterSet
//   (
//     boost::assign::list_of<ParameterSet::SingleEntry>
//       
//       ("run", new SubsetParameter	
// 	    (
// 		  ParameterSet
// 		  (
// 		    boost::assign::list_of<ParameterSet::SingleEntry>
// 		    ("machine", 	new StringParameter("", "machine or queue, where the external commands are executed on"))
// 		    ("OFEname", 	new StringParameter("OF23x", "identifier of the OpenFOAM installation, that shall be used"))
// 		    ("np", 		new IntParameter(1, "number of processors for parallel run (less or equal 1 means serial execution)"))
// // 		    ("deltaT", 		new DoubleParameter(1.0, "simulation time step"))
// // 		    ("endTime", 	new DoubleParameter(1000.0, "simulation time at which the solver should stop"))
// 		    ("mapFrom", 	new DirectoryParameter("", "Map solution from specified case, potentialinit is skipped when specified"))
// 		    ("potentialinit", 	new BoolParameter(false, "Whether to initialize the flow field by potentialFoam when no mapping is done"))
// 		    ("evaluateonly", 	new BoolParameter(false, "Whether to skip solver run and do only the evaluation"))
// 		    .convert_to_container<ParameterSet::EntryList>()
// 		  ), 
// 		  "Execution parameters"
//       ))
// 
//       ("mesh", new SubsetParameter
// 	(
// 	  ParameterSet
// 	  (
// 	    boost::assign::list_of<ParameterSet::SingleEntry>
// 	    ("linkmesh", new PathParameter("", "path to another case, from what the mesh shall be linked"))
// 	    .convert_to_container<ParameterSet::EntryList>()
// 	  ), 
// 	  "Properties of the computational mesh"
// 	))
// 
//       ("fluid", new SubsetParameter
// 	(
// 	  ParameterSet
// 	  (
// 	    boost::assign::list_of<ParameterSet::SingleEntry>
// 	    ("turbulenceModel",new SelectionParameter
// 	      (
// 		kOmegaSST2_RASModel::typeName, 
// 		turbulenceModel::factoryToC(), 
// 		"Turbulence model"
// 	      ))
// 	    .convert_to_container<ParameterSet::EntryList>()
// 	  ), 
// 	  "Parameters of the fluid"
// 	))
//       
//       ("eval", new SubsetParameter
// 	(
// 	  ParameterSet
// 	  (
// 	    boost::assign::list_of<ParameterSet::SingleEntry>
// 	    
// 	    ("reportdicts",	new BoolParameter(true, "Include dictionaries into report"))
// 	    
// 	    .convert_to_container<ParameterSet::EntryList>()
// 	  ), 
// 	  "Parameters for evaluation after solver run"
// 	))
// 
// //       ("run", new SubsetParameter	
// // 	    (
// // 		  ParameterSet
// // 		  (
// // 		    boost::assign::list_of<ParameterSet::SingleEntry>
// // 		    ("endTime", 	new DoubleParameter(1000.0, "simulation time at which the solver should stop"))
// // 		    .convert_to_container<ParameterSet::EntryList>()
// // 		  ), 
// // 		  "Solver parameters"
// //       ))
// 
//       .convert_to_container<ParameterSet::EntryList>()
//   );
}

boost::filesystem::path OpenFOAMAnalysis::setupExecutionEnvironment()
{
  path p=Analysis::setupExecutionEnvironment();
  calcDerivedInputData();
  return p;
}

void OpenFOAMAnalysis::reportIntermediateParameter(const std::string& name, double value, const std::string& description, const std::string& unit)
{
  if (!derivedInputData_)
  {
    ParameterSet empty_ps;
    derivedInputData_.reset(new ResultSet(empty_ps, "Derived Input Data", ""));
  }
  
  std::cout<<">>> Intermediate parameter "<<name<<" = "<<value<<" "<<unit;
  if (description!="")
    std::cout<<" ("<<description<<")";
  std::cout<<std::endl;
  
  boost::assign::ptr_map_insert<ScalarResult>(*derivedInputData_) (name, value, description, "", unit);
}

void OpenFOAMAnalysis::calcDerivedInputData()
{
}

void OpenFOAMAnalysis::createDictsInMemory(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  dicts=cm.createDictionaries();
}

void OpenFOAMAnalysis::applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  PSINT(parameters(), "run", np);
//   PSDBL(p, "run", deltaT);
//   PSDBL(p, "run", endTime);
// 
//   OFDictData::dict& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
//   controlDict["deltaT"]=deltaT;
//   controlDict["endTime"]=endTime;
  
  OFDictData::dict& decomposeParDict=dicts->addDictionaryIfNonexistent("system/decomposeParDict");
  if (decomposeParDict.find("numberOfSubdomains")!=decomposeParDict.end())
  {
    int cnp=boost::get<int>(decomposeParDict["numberOfSubdomains"]);
    if (cnp!=np)
    {
      insight::Warning
      (
	"decomposeParDict does not contain proper number of processors!\n"
	+str(format("(%d != %d)") % cnp % np) 
	+"\nIt will be recreated but the directional preferences cannot be taken into account.\n"
	"Correct this by setting the np parameter in FVNumerics during case creation properly."
      );
      setDecomposeParDict(*dicts, np, "scotch");
    }
  }
}

void OpenFOAMAnalysis::writeDictsToDisk(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  cm.createOnDisk(executionPath(), dicts);
  cm.modifyCaseOnDisk(executionPath());
}

void OpenFOAMAnalysis::applyCustomPreprocessing(OpenFOAMCase& cm)
{
}

void OpenFOAMAnalysis::mapFromOther(OpenFOAMCase& cm, const boost::filesystem::path& mapFromPath, bool is_parallel)
{
  if (const RASModel* rm=cm.get<RASModel>(".*"))
  {
    // check, if turbulence model is compatible in source case
    // run "createTurbulenceFields" on source case, if not
    std::string omodel=readTurbulenceModelName(mapFromPath);
    if ( (rm->type()!=omodel) && (omodel!="kOmegaSST2"))
    {
      OpenFOAMCase oc(cm.ofe());
      oc.executeCommand(mapFromPath, "createTurbulenceFields", list_of("-latestTime") );
    }
  }
  
  mapFields(cm, mapFromPath, executionPath(), is_parallel, cm.fieldNames());
}


void OpenFOAMAnalysis::initializeSolverRun(OpenFOAMCase& cm)
{
  int np=readDecomposeParDict(executionPath());
  bool is_parallel = np>1;
  
  path mapFromPath=parameters().getPath("run/mapFrom");
  
//   if (mapFromPath!="")
//   {
//     if (const RASModel* rm=cm.get<RASModel>(".*"))
//     {
//       // check, if turbulence model is compatible in source case
//       // run "createTurbulenceFields" on source case, if not
//       std::string omodel=readTurbulenceModelName(mapFromPath);
//       if (rm->type()!=omodel)
//       {
// 	OpenFOAMCase oc(cm.ofe());
// 	oc.executeCommand(mapFromPath, "createTurbulenceFields", list_of("-latestTime") );
//       }
//     }
//   }
  
  if ((cm.OFversion()>=230) && (mapFromPath!=""))
  {
    // parallelTarget option is not present in OF2.3.x
    mapFromOther(cm, mapFromPath, false);
  }

  if (is_parallel)
  {
    if (!exists(executionPath()/"processor0"))
      cm.executeCommand(executionPath(), "decomposePar");
  }
  
  if (!cm.outputTimesPresentOnDisk(executionPath()))
  {
    if ( (!(cm.OFversion()>=230)) && (mapFromPath!="") )
    {
      mapFromOther(cm, mapFromPath, is_parallel);
    }
    else
    {
      if (parameters().getBool("run/potentialinit"))
	runPotentialFoam(cm, executionPath(), &stopFlag_, np);
    }
  }
  else
  {
    cout<<"case in "<<executionPath()<<": output timestep are already there, skipping initialization."<<endl;
  }
}

void OpenFOAMAnalysis::runSolver(ProgressDisplayer* displayer, OpenFOAMCase& cm)
{
  SolverOutputAnalyzer analyzer(*displayer);
  
  string solverName;
  int np=readDecomposeParDict(executionPath());
  
  {
    OFDictData::dict controlDict;
    std::ifstream cdf( (executionPath()/"system"/"controlDict").c_str() );
    readOpenFOAMDict(cdf, controlDict);
    solverName=controlDict.getString("application");
  }

  
  std::cout<<"Executing application "<<solverName<<std::endl;
  
  cm.runSolver(executionPath(), analyzer, solverName, &stopFlag_, np);
  
}

void OpenFOAMAnalysis::finalizeSolverRun(OpenFOAMCase& cm)
{
  int np=readDecomposeParDict(executionPath());
  bool is_parallel = np>1;
  if (is_parallel)
  {
    if (exists(executionPath()/"processor0"))
      cm.executeCommand(executionPath(), "reconstructPar", list_of<string>("-latestTime") );
    else
      insight::Warning("A parallel run is configured, but not processor directory is present!\nProceeding anyway.");
  }
}

ResultSetPtr OpenFOAMAnalysis::evaluateResults(OpenFOAMCase& cm)
{
  ResultSetPtr results(new ResultSet(parameters(), name_, "Result Report"));
  
  meshQualityReport(cm, executionPath(), results);
  
  if (parameters().getBool("eval/reportdicts"))
  {
    currentNumericalSettingsReport(cm, executionPath(), results);
  }
  
  if (derivedInputData_)
  {
    std::string key(derivedInputData_->title());
    results->insert( key, derivedInputData_->clone() );
  }
  
  return results;
}

void OpenFOAMAnalysis::createCaseOnDisk(OpenFOAMCase& runCase)
{
  const ParameterSet& p = *parameters_;
  
  PSSTR(p, "run", machine);
  PSSTR(p, "run", OFEname);
  
  OFEnvironment ofe = OFEs::get(OFEname);
  ofe.setExecutionMachine(machine);
  
  path dir = setupExecutionEnvironment();

  if (p.getBool("run/evaluateonly"))
    cout<< "Parameter \"run/evaluateonly\" is set: SKIPPING SOLVER RUN AND PROCEEDING WITH EVALUATION!" <<endl;

  boost::shared_ptr<OpenFOAMCase> meshCase;
  bool meshcreated=false;
  if (!p.getBool("run/evaluateonly"))
  {
    //p.saveToFile(dir/"parameters.ist", type());
    
    {
      meshCase.reset(new OpenFOAMCase(ofe));
      if (!meshCase->meshPresentOnDisk(dir))
      {
	meshcreated=true;
	if (!p.getPath("mesh/linkmesh").empty())
	{
	  linkPolyMesh(p.getPath("mesh/linkmesh")/"constant", dir/"constant", &ofe);
	}
	else
	{
	  createMesh(*meshCase);
// 	  meshcreated=true;
	}
      }
      else
	cout<<"case in "<<dir<<": mesh is already there, skipping mesh creation."<<endl;
    }
  }

  createCase(runCase);
  boost::shared_ptr<OFdicts> dicts;
  createDictsInMemory(runCase, dicts);
  applyCustomOptions(runCase, dicts);
  
  if (!runCase.outputTimesPresentOnDisk(dir))
  {
    if (meshcreated) 
      runCase.modifyMeshOnDisk(executionPath());
    writeDictsToDisk(runCase, dicts);
    applyCustomPreprocessing(runCase);
  }
  else
    cout<<"case in "<<dir<<": output timestep are already there, skipping case recreation."<<endl;    

}


ResultSetPtr OpenFOAMAnalysis::operator()(ProgressDisplayer* displayer)
{  
  const ParameterSet& p = *parameters_;
  
  PSSTR(p, "run", machine);
  PSSTR(p, "run", OFEname);
  
  OFEnvironment ofe = OFEs::get(OFEname);
  ofe.setExecutionMachine(machine);

  OpenFOAMCase runCase(ofe);
  createCaseOnDisk(runCase);
  
  path dir = executionPath();
  
  if (!p.getBool("run/evaluateonly"))
  {
    initializeSolverRun(runCase);
    runSolver(displayer, runCase);
  }
  
  finalizeSolverRun(runCase);

  return evaluateResults(runCase);
}



defineType(OpenFOAMParameterStudy);

OpenFOAMParameterStudy::OpenFOAMParameterStudy
(
    const std::string& name, 
    const std::string& description, 
    const OpenFOAMAnalysis& baseAnalysis, 
    const RangeParameterList& varp,
    bool subcasesRemesh
)
: ParameterStudy
  (
    name, description, baseAnalysis, varp
  ),
  subcasesRemesh_(subcasesRemesh)
{
}

void OpenFOAMParameterStudy::modifyInstanceParameters(const std::string& subcase_name, ParameterSetPtr& newp) const
{
  boost::filesystem::path oldmf = newp->get<PathParameter>("run/mapFrom")();
  boost::filesystem::path newmf = "";
  if (oldmf!="")
  {
    oldmf=boost::filesystem::absolute(oldmf);
    newmf = oldmf / subcase_name;
    if (!boost::filesystem::exists(newmf)) 
    {
      insight::Warning("No matching subcase exists in "+oldmf.string()+" for mapping of subcase "+subcase_name+"! Omitting.");
      newmf="";
    }
  }
  newp->get<PathParameter>("run/mapFrom")() = newmf;
}


ResultSetPtr OpenFOAMParameterStudy::operator()(ProgressDisplayer* displayer)
{
  // generate the mesh in the top level case first
  ParameterSet& p = *parameters_;    

  path dir = setupExecutionEnvironment();
  //p.saveToFile(dir/"parameters.ist", type());

  {  
    OpenFOAMAnalysis* base_case=static_cast<OpenFOAMAnalysis*>(baseAnalysis_.get());
    
    PSSTR(p, "run", machine);
    PSSTR(p, "run", OFEname);
    
    OFEnvironment ofe = OFEs::get(OFEname);
    ofe.setExecutionMachine(machine);
    
    path exep=executionPath();
    
    // Generate a valid parameterset with actual values for mesh mesh genration
    // use first value from each range
    ParameterSet defp(p);
    for (int j=0; j<varp_.size(); j++)
    {
      // Replace RangeParameter by first actual single value
      const DoubleRangeParameter& rp = p.get<DoubleRangeParameter>(varp_[j]);
      DoubleParameter* dp=rp.toDoubleParameter(rp.values().begin());
      defp.replace(varp_[j], dp);
    }
    base_case->setParameters(defp);
    
    base_case->setExecutionPath(exep);
    dir = base_case->setupExecutionEnvironment();

    if (!subcasesRemesh_)
    {
      OpenFOAMCase meshCase(ofe);
      if (!meshCase.meshPresentOnDisk(dir))
	base_case->createMesh(meshCase);
      else
	cout<<"case in "<<dir<<": mesh is already there, skipping mesh creation."<<endl;
    }
  }
  
  path old_lp=p.get<PathParameter>("mesh/linkmesh")();
  if (!subcasesRemesh_)
    p.get<PathParameter>("mesh/linkmesh")() = boost::filesystem::absolute(executionPath());
  setupQueue();
  p.get<PathParameter>("mesh/linkmesh")() = old_lp;
  
  processQueue(displayer);
  ResultSetPtr results = evaluateRuns();

  evaluateCombinedResults(results);
  
  return results;
}

void OpenFOAMParameterStudy::evaluateCombinedResults(ResultSetPtr& results)
{
}

}

