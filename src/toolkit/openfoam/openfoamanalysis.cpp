/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  Hannes Kroeger <email>

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


#include "openfoamanalysis.h"
#include "basiccaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"
#include "boost/filesystem.hpp"

using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;
using namespace std;

namespace insight
{

void insertTurbulenceModel(OpenFOAMCase& cm, const std::string& name)
{
  turbulenceModel* model = turbulenceModel::lookup(name, cm);
  
  if (!model) 
    throw insight::Exception("Unrecognized RASModel selection: "+name);
  
  cm.insert(model);
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

  return ParameterSet
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
      
      ("run", new SubsetParameter	
	    (
		  ParameterSet
		  (
		    boost::assign::list_of<ParameterSet::SingleEntry>
		    ("machine", 	new StringParameter("", "machine or queue, where the external commands are executed on"))
		    ("OFEname", 	new StringParameter("OF22x", "identifier of the OpenFOAM installation, that shall be used"))
		    ("np", 	new IntParameter(1, "number of processors for parallel run, <=1 means serial execution"))
		    .convert_to_container<ParameterSet::EntryList>()
		  ), 
		  "Execution parameters"
      ))
         
      ("solver", new SubsetParameter	
	    (
		  ParameterSet
		  (
		    boost::assign::list_of<ParameterSet::SingleEntry>
		    ("endTime", 	new DoubleParameter(1000.0, "simulation time at which the solver should stop"))
		    .convert_to_container<ParameterSet::EntryList>()
		  ), 
		  "Solver parameters"
      ))

      .convert_to_container<ParameterSet::EntryList>()
  );
}

void OpenFOAMAnalysis::createDictsInMemory(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  dicts=cm.createDictionaries();
}

void OpenFOAMAnalysis::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  PSINT(p, "run", np);
  PSDBL(p, "run", endTime);

  OFDictData::dict& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  controlDict["endTime"]=endTime;
  
  OFDictData::dict& decomposeParDict=dicts->addDictionaryIfNonexistent("system/decomposeParDict");
  decomposeParDict["numberOfSubdomains"]=np;
  decomposeParDict["method"]="scotch";
}

void OpenFOAMAnalysis::writeDictsToDisk(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  cm.createOnDisk(executionPath(), dicts);
}

void OpenFOAMAnalysis::applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p)
{
}

void OpenFOAMAnalysis::runSolver(ProgressDisplayer* displayer, OpenFOAMCase& cm, const ParameterSet& p)
{
  SolverOutputAnalyzer analyzer(*displayer);
  
  string solverName;
  int np;
  
  {
    OFDictData::dict controlDict;
    std::ifstream cdf( (executionPath()/"system"/"controlDict").c_str() );
    readOpenFOAMDict(cdf, controlDict);
    solverName=controlDict.getString("application");
  }
  {
    OFDictData::dict decomposeParDict;
    std::ifstream cdf( (executionPath()/"system"/"decomposeParDict").c_str() );
    readOpenFOAMDict(cdf, decomposeParDict);
    np=decomposeParDict.getInt("numberOfSubdomains");
  }
  
  std::cout<<"Executing application "<<solverName<<std::endl;
  
  if (np>1)
  {
    cm.executeCommand(executionPath(), "decomposePar");
  }
  
  cm.runSolver(executionPath(), analyzer, solverName, &stopFlag_, np);
  
  if (np>1)
  {
    cm.executeCommand(executionPath(), "reconstructPar", list_of<string>("-latestTime") );
  }
}

ResultSetPtr OpenFOAMAnalysis::evaluateResults(OpenFOAMCase& cm, const ParameterSet& p)
{
  ResultSetPtr results(new ResultSet(p, name_, "Result Report"));
  return results;
}

ResultSetPtr OpenFOAMAnalysis::operator()(ProgressDisplayer* displayer)
{  
  const ParameterSet& p = *parameters_;
  
  PSSTR(p, "run", machine);
  PSSTR(p, "run", OFEname);
  
  OFEnvironment ofe = OFEs::get(OFEname);
  ofe.setExecutionMachine(machine);
  
  path dir = setupExecutionEnvironment();

  p.saveToFile(dir/"parameters.ist", type());
  
  {
    OpenFOAMCase meshCase(ofe);
    if (!meshCase.meshPresentOnDisk(dir))
      createMesh(meshCase, p);
    else
      cout<<"case in "<<dir<<": mesh is already there, skipping mesh creation."<<endl;
  }

  OpenFOAMCase runCase(ofe);
  if (!runCase.outputTimesPresentOnDisk(dir))
  {
    createCase(runCase, p);
  }
  else
    cout<<"case in "<<dir<<": output timestep are already there, skipping case recreation and run."<<endl;    
  
  boost::shared_ptr<OFdicts> dicts;
  createDictsInMemory(runCase, p, dicts);
  applyCustomOptions(runCase, p, dicts);
  writeDictsToDisk(runCase, p, dicts);
  applyCustomPreprocessing(runCase, p);
  
  runSolver(displayer, runCase, p);
  
  return evaluateResults(runCase, p);
}

}

