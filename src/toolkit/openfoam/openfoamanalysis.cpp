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


#include "openfoam/caseelements/basic/decomposepardict.h"
#include "openfoam/caseelements/turbulencemodel.h"
#include "openfoam/caseelements/turbulencemodelcaseelements.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/ofes.h"

#include "openfoamtools.h"
#include "openfoamanalysis.h"

#include "base/boost_include.h"

using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;
using namespace std;

namespace insight
{

turbulenceModel* insertTurbulenceModel(OpenFOAMCase& cm, const OpenFOAMAnalysis::Parameters& params)
{
  CurrentExceptionContext ex("inserting turbulence model configuration into OpenFOAM case");

  const OpenFOAMAnalysis::Parameters::fluid_type::turbulenceModel_type& tmp
      = params.fluid.turbulenceModel;

  turbulenceModel* model = turbulenceModel::lookup(tmp.selection, cm, tmp.parameters);

  if (!model)
    throw insight::Exception("Unrecognized RASModel selection: "+tmp.selection);

  return cm.insert(model);
}

turbulenceModel* insertTurbulenceModel(OpenFOAMCase& cm, const SelectableSubsetParameter& ps)
{
  CurrentExceptionContext ex("inserting turbulence model configuration into OpenFOAM case");

  turbulenceModel* model = turbulenceModel::lookup(ps.selection(), cm, ps());
  
  if (!model) 
    throw insight::Exception("Unrecognized RASModel selection: "+ps.selection());
  
  return cm.insert(model);
}



OpenFOAMAnalysis::OpenFOAMAnalysis
(        
    const std::string& name,
    const std::string& description,
    const ParameterSet& ps,
    const boost::filesystem::path& exepath
)
: Analysis(name, description, ps, exepath)
{
}

boost::filesystem::path OpenFOAMAnalysis::setupExecutionEnvironment()
{
  CurrentExceptionContext ex("creating the execution directory");

  path p=Analysis::setupExecutionEnvironment();
  
//   writestepcache_=true;
//   if (exists(stepcachefile_))
//   {
//     std::ifstream stc(stepcachefile_.c_str());
//     while (!stc.eof())
//     {
//       std::string stepname;
//       getline(stc, stepname);
//       performedsteps_.insert(stepname);
//     }
//   }
  
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

void OpenFOAMAnalysis::createDictsInMemory(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
{
  CurrentExceptionContext ex("creating OpenFOAM dictionaries in memory for case \""+executionPath().string()+"\"");
  dicts=cm.createDictionaries();
}

void OpenFOAMAnalysis::applyCustomOptions(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
{
  CurrentExceptionContext ex("applying custom options to OpenFOAM case configuration for case \""+executionPath().string()+"\"");

  Parameters p(parameters_);
  
  OFDictData::dict& dpd=dicts->lookupDict("system/decomposeParDict");
  if (dpd.find("numberOfSubdomains")!=dpd.end())
  {
    int cnp=boost::get<int>(dpd["numberOfSubdomains"]);
    if (cnp!=p.run.np)
    {
      insight::Warning
      (
	"decomposeParDict does not contain proper number of processors!\n"
	+str(format("(%d != %d)") % cnp % p.run.np) 
	+"\nIt will be recreated but the directional preferences cannot be taken into account.\n"
	"Correct this by setting the np parameter in FVNumerics during case creation properly."
      );
      decomposeParDict(cm, decomposeParDict::Parameters()
                        .set_np(p.run.np)
                        .set_decompositionMethod(decomposeParDict::Parameters::decompositionMethod_type::scotch)
                       ).addIntoDictionaries(*dicts);
    }
  }
}

void OpenFOAMAnalysis::writeDictsToDisk(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
{
  CurrentExceptionContext ex("writing OpenFOAM dictionaries to case \""+executionPath().string()+"\"");

  cm.createOnDisk(executionPath(), dicts);
  cm.modifyCaseOnDisk(executionPath());
}

void OpenFOAMAnalysis::applyCustomPreprocessing(OpenFOAMCase&)
{
}

void OpenFOAMAnalysis::mapFromOther(OpenFOAMCase& cm, const boost::filesystem::path& mapFromPath, bool is_parallel)
{
  CurrentExceptionContext ex("mapping existing CFD solution from case \""+mapFromPath.string()+"\" to case \""+executionPath().string()+"\"");

  if (const RASModel* rm=cm.get<RASModel>(".*"))
  {
    // check, if turbulence model is compatible in source case
    // run "createTurbulenceFields" on source case, if not
    try
    {
        OpenFOAMCase oc(cm.ofe());
        std::string omodel=readTurbulenceModelName(oc, mapFromPath);
        if ( (rm->type()!=omodel) && (omodel!="kOmegaSST2"))
        {
          CurrentExceptionContext ex("converting turbulence quantities in case \""+mapFromPath.string()+"\" since the turbulence model is different.");
          oc.executeCommand(mapFromPath, "createTurbulenceFields", list_of("-latestTime") );
        }
    }
    catch (...)
    {
        insight::Warning("could not check turbulence model of source case. Continuing anyway.");
    }
  }
  
  mapFields(cm, mapFromPath, executionPath(), is_parallel, cm.fieldNames());
}


void OpenFOAMAnalysis::initializeSolverRun(ProgressDisplayer*, OpenFOAMCase& cm)
{
  CurrentExceptionContext ex("initializing solver run for case \""+executionPath().string()+"\"");

  Parameters p(parameters_);
    
  int np=readDecomposeParDict(executionPath());
  bool is_parallel = np>1;
    
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
  
  if (!cm.outputTimesPresentOnDisk(executionPath(), false))
  {
    if ((cm.OFversion()>=230) && (p.run.mapFrom->isValid()))
    {
      // parallelTarget option is not present in OF2.3.x
      mapFromOther(cm, p.run.mapFrom->filePath(executionPath()), false);
    }
  }

  if (is_parallel)
  {
    if (!exists(executionPath()/"processor0"))
      cm.executeCommand(executionPath(), "decomposePar");
  }
  
  if (!cm.outputTimesPresentOnDisk(executionPath(), is_parallel))
  {
    if ( (!(cm.OFversion()>=230)) && (p.run.mapFrom->isValid()) )
    {
      mapFromOther(cm, p.run.mapFrom->filePath(executionPath()), is_parallel);
    }
    else
    {
      if (p.run.potentialinit)
        runPotentialFoam(cm, executionPath(), np);
    }
  }
  else
  {
    cout<<"case in "<<executionPath()<<": output timestep are already there, skipping initialization."<<endl;
  }
}

void OpenFOAMAnalysis::installConvergenceAnalysis(std::shared_ptr<ConvergenceAnalysisDisplayer> cc)
{
  convergenceAnalysis_.push_back(cc);
}


void OpenFOAMAnalysis::runSolver(ProgressDisplayer* displayer, OpenFOAMCase& cm)
{
  CurrentExceptionContext ex("running solver");

  CombinedProgressDisplayer cpd(CombinedProgressDisplayer::OR), conv(CombinedProgressDisplayer::AND);
  if (displayer) cpd.add(displayer);
  cpd.add(&conv);
  
  for (decltype(convergenceAnalysis_)::value_type& ca: convergenceAnalysis_)
  {
    conv.add(ca.get());
  }
  SolverOutputAnalyzer analyzer(cpd);
  
  string solverName;
  int np=readDecomposeParDict(executionPath());
  
  {
    OFDictData::dict controlDict;
    std::ifstream cdf( (executionPath()/"system"/"controlDict").c_str() );
    readOpenFOAMDict(cdf, controlDict);
    solverName=controlDict.getString("application");
  }

  
  std::cout<<"Executing application "<<solverName<<std::endl;
  
  cm.runSolver(executionPath(), analyzer, solverName, np);
  
}

void OpenFOAMAnalysis::finalizeSolverRun(OpenFOAMCase& cm)
{
  CurrentExceptionContext ex("finalizing solver run for case \""+executionPath().string()+"\"");

  int np=readDecomposeParDict(executionPath());
  bool is_parallel = np>1;
  if (is_parallel)
  {
    if (exists(executionPath()/"processor0"))
    {
        if (checkIfReconstructLatestTimestepNeeded(cm, executionPath()))
        {
            cm.executeCommand(executionPath(), "reconstructPar", list_of<string>("-latestTime") );
        }
//       cm.removeProcessorDirectories(executionPath());  //will remove proc dirs, if evaluation is executed while solution is still running...
    }
    else
      insight::Warning("A parallel run is configured, but not processor directory is present!\nProceeding anyway.");
  }
}

ResultSetPtr OpenFOAMAnalysis::evaluateResults(OpenFOAMCase& cm)
{
  CurrentExceptionContext ex("evaluating the results for case \""+executionPath().string()+"\"");


  Parameters p(parameters_);
  
  ResultSetPtr results(new ResultSet(parameters(), name_, "Result Report"));
  results->introduction() = description_;
  
  if (!p.eval.skipmeshquality)
  {
    meshQualityReport(cm, executionPath(), results);
  }
  
  if (parameters().getBool("eval/reportdicts"))
  {
    currentNumericalSettingsReport(cm, executionPath(), results);
  }
  
  if (derivedInputData_)
  {
    std::string key(derivedInputData_->title());
    results->insert( key, derivedInputData_->clone() ) .setOrder(-1.);
  }
  
  return results;
}




void OpenFOAMAnalysis::createCaseOnDisk(OpenFOAMCase& runCase)
{
  CurrentExceptionContext ex("creating OpenFOAM case in directory \""+executionPath().string()+"\"");

    Parameters p(parameters_);

    OFEnvironment ofe = OFEs::get(p.run.OFEname);
    ofe.setExecutionMachine(p.run.machine);

    path dir = setupExecutionEnvironment();

    bool evaluateonly=p.run.evaluateonly;
    if (evaluateonly)
        cout<< "Parameter \"run/evaluateonly\" is set: SKIPPING SOLVER RUN AND PROCEEDING WITH EVALUATION!" <<endl;

    std::shared_ptr<OpenFOAMCase> meshCase;
    bool meshcreated=false;
    if (!evaluateonly)
    {
        //p.saveToFile(dir/"parameters.ist", type());

        {
            meshCase.reset(new OpenFOAMCase(ofe));
            if (!meshCase->meshPresentOnDisk(dir))
            {
                meshcreated=true;
                if (p.mesh.linkmesh->isValid())
                {
                  linkPolyMesh(p.mesh.linkmesh->filePath(executionPath())/"constant", dir/"constant", &ofe);
                }
                else
                {
                  createMesh(*meshCase);
                }
            }
            else
                cout<<"case in "<<dir<<": mesh is already there, skipping mesh creation."<<endl;
        }
    }

    createCase(runCase);
    std::shared_ptr<OFdicts> dicts;
    createDictsInMemory(runCase, dicts);
    applyCustomOptions(runCase, dicts);

    int np=1;
    if (boost::filesystem::exists(executionPath()/"system"/"decomposeParDict"))
    {
        np=readDecomposeParDict(executionPath());
    }
    bool is_parallel = np>1;
    if (!runCase.outputTimesPresentOnDisk(dir, is_parallel) && !evaluateonly)
    {
        if (meshcreated)
            runCase.modifyMeshOnDisk(executionPath());
        writeDictsToDisk(runCase, dicts);
        applyCustomPreprocessing(runCase);
    }
    else
        cout<<"case in "<<dir<<": skipping case recreation."<<endl;

}




ResultSetPtr OpenFOAMAnalysis::operator()(ProgressDisplayer* displayer)
{  
  CurrentExceptionContext ex("running OpenFOAM analysis in directory \""+executionPath().string()+"\"");

  Parameters p(parameters_);
  
  OFEnvironment ofe = OFEs::get(p.run.OFEname);
  ofe.setExecutionMachine(p.run.machine);

  OpenFOAMCase runCase(ofe);
  createCaseOnDisk(runCase);
  
  path dir = executionPath();
  
  if (!p.run.evaluateonly)
  {
    PrefixedProgressDisplayer iniprogdisp(displayer, "initrun_");
    initializeSolverRun(&iniprogdisp, runCase);
    runSolver(displayer, runCase);
  }
  
  finalizeSolverRun(runCase);

  return evaluateResults(runCase);
}



}

