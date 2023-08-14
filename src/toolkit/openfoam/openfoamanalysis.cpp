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
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/ofes.h"
#include "base/progressdisplayer/combinedprogressdisplayer.h"
#include "base/progressdisplayer/convergenceanalysisdisplayer.h"
#include "base/progressdisplayer/prefixedprogressdisplayer.h"

#include "openfoam/caseelements/basic/rasmodel.h"

#include "openfoamtools.h"
#include "openfoamanalysis.h"

#include "base/boost_include.h"
#include "base/translations.h"

using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;
using namespace std;

namespace insight
{




turbulenceModel* insertTurbulenceModel(OpenFOAMCase& cm, const SelectableSubsetParameter& ps)
{
  CurrentExceptionContext ex("inserting turbulence model configuration into OpenFOAM case");
  struct P { std::string selection; ParameterSet parameters; };
  return insertTurbulenceModel(cm, P{ ps.selection(), ps() } );
}





OpenFOAMAnalysis::OpenFOAMAnalysis
(        
    const std::string& name,
    const std::string& description,
    const ParameterSet& ps,
    const boost::filesystem::path& exepath
)
: Analysis(name, description, ps, exepath),
  p_(ps)
{}




boost::filesystem::path OpenFOAMAnalysis::setupExecutionEnvironment()
{
  CurrentExceptionContext ex("creating the execution directory");

  path p=Analysis::setupExecutionEnvironment();
  
  return p;
}



void OpenFOAMAnalysis::initializeDerivedInputDataSection()
{
  if (!derivedInputData_)
  {
    ParameterSet empty_ps;
    derivedInputData_.reset(new ResultSet(empty_ps, "Derived Input Data", ""));
  }
}

void OpenFOAMAnalysis::reportIntermediateParameter(const std::string& name, double value, const std::string& description, const std::string& unit)
{
  initializeDerivedInputDataSection();

  std::cout<<">>> Intermediate parameter "<<name<<" = "<<value<<" "<<unit;
  if (description!="")
    std::cout<<" ("<<description<<")";
  std::cout<<std::endl;
  
  boost::assign::ptr_map_insert<ScalarResult>(*derivedInputData_) (name, value, description, "", unit);
}




void OpenFOAMAnalysis::calcDerivedInputData(ProgressDisplayer&)
{}




void OpenFOAMAnalysis::createDictsInMemory(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
{
  CurrentExceptionContext ex("creating OpenFOAM dictionaries in memory for case \""+executionPath().string()+"\"");
  dicts=cm.createDictionaries();
}




void OpenFOAMAnalysis::applyCustomOptions(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
{
  CurrentExceptionContext ex("applying custom options to OpenFOAM case configuration for case \""+executionPath().string()+"\"");


  OFDictData::dict& dpd=dicts->lookupDict("system/decomposeParDict");
  if (dpd.find("numberOfSubdomains")!=dpd.end())
  {
    int cnp=boost::get<int>(dpd["numberOfSubdomains"]);
    if (cnp!=p_.run.np)
    {
      insight::Warning
      (
        "decomposeParDict does not contain proper number of processors!\n"
        +str(format("(%d != %d)\n") % cnp % p_.run.np)
        +"It will be recreated but the directional preferences cannot be taken into account.\n"
	"Correct this by setting the np parameter in FVNumerics during case creation properly."
      );
      decomposeParDict(cm, decomposeParDict::Parameters()
                        .set_np(p_.run.np)
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

void OpenFOAMAnalysis::applyCustomPreprocessing(OpenFOAMCase&, ProgressDisplayer&)
{}


void OpenFOAMAnalysis::changeMapFromPath(const boost::filesystem::path &newMapFromPath)
{
  p_.run.mapFrom->setOriginalFilePath(newMapFromPath);
}

void OpenFOAMAnalysis::mapFromOther(OpenFOAMCase& cm, ProgressDisplayer& parentAction, const boost::filesystem::path& mapFromPath, bool is_parallel)
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
          parentAction.message(ex);
          oc.executeCommand(mapFromPath, "createTurbulenceFields", list_of("-latestTime") );
        }
    }
    catch (...)
    {
        insight::Warning("could not check turbulence model of source case. Continuing anyway.");
    }
  }
  
  parentAction.message("Executing mapFields");
  mapFields(cm, mapFromPath, executionPath(), is_parallel, cm.fieldNames());
}


void OpenFOAMAnalysis::initializeSolverRun(ProgressDisplayer& parentProgress, OpenFOAMCase& cm)
{
  CurrentExceptionContext ex("initializing solver run for case \""+executionPath().string()+"\"");

  int np=readDecomposeParDict(executionPath());
  bool is_parallel = np>1;

  
  if (!cm.outputTimesPresentOnDisk(executionPath(), false))
  {
    if ((cm.OFversion()>=230) && (p_.run.mapFrom->isValid()))
    {
      // parallelTarget option is not present in OF2.3.x
      mapFromOther(cm, parentProgress, p_.run.mapFrom->filePath(executionPath()), false);
    }
  }

  if (is_parallel)
  {
    if (!exists(executionPath()/"processor0"))
    {
      parentProgress.message("Executing decomposePar");

      std::vector<std::string> opts;
      if (exists(executionPath()/"constant"/"regionProperties"))
        opts={"-allRegions"};

      cm.executeCommand(executionPath(), "decomposePar", opts);
    }
  }
  
  if (!cm.outputTimesPresentOnDisk(executionPath(), is_parallel))
  {
    if ( (!(cm.OFversion()>=230)) && (p_.run.mapFrom->isValid()) )
    {
      mapFromOther(cm, parentProgress, p_.run.mapFrom->filePath(executionPath()), is_parallel);
    }
    else
    {
      if (p_.run.potentialinit)
      {
        parentProgress.message("Executing potentialFoam");
        runPotentialFoam(cm, executionPath(), np);
      }
    }
  }
  else
  {
    parentProgress.message("case in "+executionPath().string()+": output timestep are already there, skipping initialization.");
  }
}

void OpenFOAMAnalysis::installConvergenceAnalysis(std::shared_ptr<ConvergenceAnalysisDisplayer> cc)
{
  convergenceAnalysis_.push_back(cc);
}

void OpenFOAMAnalysis::prepareCaseCreation(ProgressDisplayer &/*progress*/)
{
  // do nothing by default
}


void OpenFOAMAnalysis::runSolver(ProgressDisplayer& parentProgress, OpenFOAMCase& cm)
{
  CurrentExceptionContext ex("running solver");

  CombinedProgressDisplayer cpd(CombinedProgressDisplayer::OR), conv(CombinedProgressDisplayer::AND);
  cpd.add(&parentProgress);
  cpd.add(&conv);
  
  for (decltype(convergenceAnalysis_)::value_type& ca: convergenceAnalysis_)
  {
    conv.add(ca.get());
  }

  
  string solverName;
  double endTime;
  int np=readDecomposeParDict(executionPath());
  
  {
    OFDictData::dict controlDict;
    std::ifstream cdf( (executionPath()/"system"/"controlDict").c_str() );
    readOpenFOAMDict(cdf, controlDict);
    solverName=controlDict.getString("application");
    endTime=controlDict.getDoubleOrInt("endTime");
  }

  SolverOutputAnalyzer analyzer(cpd, endTime);
  
  parentProgress.message( str(format("Executing application %s until end time %g.") % solverName % endTime) );
  
  cm.runSolver(executionPath(), analyzer, solverName, np);
  
}

void OpenFOAMAnalysis::finalizeSolverRun(OpenFOAMCase& cm, ProgressDisplayer& parentAction)
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
          parentAction.message("Running reconstructPar for latest time step");

          std::vector<std::string> opts={"-latestTime"};
          if (exists(executionPath()/"constant"/"regionProperties"))
            opts.push_back("-allRegions");

          cm.executeCommand(executionPath(), "reconstructPar", opts );
        }
        else
        {
          parentAction.message("No reconstruct needed");
        }
    }
    else
      insight::Warning("A parallel run is configured, but not processor directory is present!\nProceeding anyway.");
  }
}

ResultSetPtr OpenFOAMAnalysis::evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress)
{
  CurrentExceptionContext ex( _("evaluating the results for case \"%s\""), executionPath().string().c_str() );

  auto results = std::make_shared<ResultSet>(parameters(), name_, "Result Report");
  results->introduction() = description_;
  
  if (!p_.eval.skipmeshquality)
  {
    parentActionProgress.message("Generating mesh quality report");
    meshQualityReport(cm, executionPath(), results);
  }
  
  if (p_.eval.reportdicts)
  {
    parentActionProgress.message("Adding numerical settings to report");
    currentNumericalSettingsReport(cm, executionPath(), results);
  }
  
  if (derivedInputData_)
  {
    parentActionProgress.message("Inserting derived input quantities into report");
    std::string key(derivedInputData_->title());
    results->insert( key, derivedInputData_->clone() ) .setOrder(-1.);
  }
  
  return results;
}




void OpenFOAMAnalysis::createCaseOnDisk(OpenFOAMCase& runCase, ProgressDisplayer& parentActionProgress)
{
  path dir = executionPath();

  CurrentExceptionContext ex("creating OpenFOAM case in directory \""+dir.string()+"\"");

    OFEnvironment ofe = OFEs::get(p_.run.OFEname);
    ofe.setExecutionMachine(p_.run.machine);

    parentActionProgress.message("Computing derived input quantities");
    calcDerivedInputData(parentActionProgress);

    bool evaluateonly=p_.run.evaluateonly;
    if (evaluateonly)
    {
      insight::Warning("Parameter \"run/evaluateonly\" is set.\nSKIPPING SOLVER RUN AND PROCEEDING WITH EVALUATION!");
    }

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
                if (p_.mesh.linkmesh->isValid())
                {
                  parentActionProgress.message("Linking the mesh to OpenFOAM case in directory "+dir.string()+".");
                  linkPolyMesh(p_.mesh.linkmesh->filePath(executionPath())/"constant", dir/"constant", &ofe);
                }
                else
                {
                  parentActionProgress.message("Creating the mesh.");
                  createMesh(*meshCase, parentActionProgress);
                }
            }
            else
            {
              insight::Warning("case in "+dir.string()+": mesh is already there, skipping mesh creation.");
            }
        }
    }

    int np=1;
    if (boost::filesystem::exists(executionPath()/"system"/"decomposeParDict"))
    {
        np=readDecomposeParDict(executionPath());
    }
    bool is_parallel = np>1;

    if (!runCase.outputTimesPresentOnDisk(dir, is_parallel) && !evaluateonly)
    {
      runCase.modifyFilesOnDiskBeforeDictCreation( executionPath() );
    }


    parentActionProgress.message("Creating the case setup.");
    createCase(runCase, parentActionProgress);

    std::shared_ptr<OFdicts> dicts;

    parentActionProgress.message("Creating the dictionaries.");
    createDictsInMemory(runCase, dicts);

    parentActionProgress.message("Applying custom modifications to dictionaries.");
    applyCustomOptions(runCase, dicts);

    // might have changed
    if (boost::filesystem::exists(executionPath()/"system"/"decomposeParDict"))
    {
        np=readDecomposeParDict(executionPath());
    }
    is_parallel = np>1;

    if (!runCase.outputTimesPresentOnDisk(dir, is_parallel) && !evaluateonly)
    {
        if (meshcreated)
        {
          parentActionProgress.message("Applying custom modifications mesh.");
          runCase.modifyMeshOnDisk(executionPath());
        }

        parentActionProgress.message("Writing dictionaries to disk.");
        writeDictsToDisk(runCase, dicts);

        parentActionProgress.message("Applying custom preprocessing steps to OpenFOAM case.");
        applyCustomPreprocessing(runCase, parentActionProgress);
    }
    else
        insight::Warning("case in "+dir.string()+": skipping case recreation because there are already output time directories present.");

}




ResultSetPtr OpenFOAMAnalysis::operator()(ProgressDisplayer& progress)
{  
  CurrentExceptionContext ex("running OpenFOAM analysis");

  auto ofprg = progress.forkNewAction( p_.run.evaluateonly? 5 : 7 );

  ofprg.message("Creating execution environment");
  setupExecutionEnvironment();
  ++ofprg;
  
  OFEnvironment ofe = OFEs::get(p_.run.OFEname);
  ofe.setExecutionMachine(p_.run.machine);

  ofprg.message("Preparing case creation");
  prepareCaseCreation(ofprg);
  ++ofprg;

  OpenFOAMCase runCase(ofe);
  ofprg.message("Creating case on disk");
  createCaseOnDisk(runCase, ofprg);
  ++ofprg;
  
  path dir = executionPath();
  
  if (!p_.run.evaluateonly)
  {
    PrefixedProgressDisplayer iniprogdisp(
          &ofprg, "initrun",
          PrefixedProgressDisplayer::Prefixed,
          PrefixedProgressDisplayer::NoActionProgressPrefix
         );

    ofprg.message("Initializing solver run");
    initializeSolverRun(iniprogdisp, runCase);
    ++ofprg;

    ofprg.message("Running solver");
    runSolver(ofprg, runCase);
    ++ofprg;
  }
  
  ofprg.message("Finalizing solver run");
  finalizeSolverRun(runCase, ofprg);
  ++ofprg;

  ofprg.message("Evaluating results");
  auto results = evaluateResults(runCase, ofprg);
  ++ofprg;

  return results;
}



}

