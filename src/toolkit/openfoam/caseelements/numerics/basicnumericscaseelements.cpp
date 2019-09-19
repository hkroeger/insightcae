#include "basicnumericscaseelements.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"


#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{




defineType(FVNumerics);



FVNumerics::FVNumerics(OpenFOAMCase& c, const ParameterSet& ps, const std::string& pName)
: decomposeParDict(c, ps),
  p_(ps),
  pName_(pName)
{
  rename("FVNumerics");
}



void FVNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
//  std::cerr<<"Addd FVN "<<p_.decompWeights<<std::endl;
  // setup structure of dictionaries
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["deltaT"]=p_.deltaT;
  controlDict["maxCo"]=0.5;
  controlDict["startFrom"]="latestTime";
  controlDict["startTime"]=0.0;
  controlDict["stopAt"]="endTime";
  controlDict["endTime"]=p_.endTime;

  std::string wfc("UNDEFINED");
  if (p_.writeControl == Parameters::writeControl_type::adjustableRunTime)
  {
      wfc="adjustableRunTime";
  }
  else if (p_.writeControl == Parameters::writeControl_type::clockTime)
  {
      wfc="clockTime";
  }
  else if (p_.writeControl == Parameters::writeControl_type::cpuTime)
  {
      wfc="cpuTime";
  }
  else if (p_.writeControl == Parameters::writeControl_type::runTime)
  {
      wfc="runTime";
  }
  else if (p_.writeControl == Parameters::writeControl_type::timeStep)
  {
      wfc="timeStep";
  }
  controlDict["writeControl"]=wfc;

  controlDict["writeInterval"]=p_.writeInterval;
  controlDict["purgeWrite"]=p_.purgeWrite;

  std::string wfk("UNDEFINED");
  if (p_.writeFormat == Parameters::writeFormat_type::ascii)
  {
      wfk="ascii";
  }
  else if (p_.writeFormat == Parameters::writeFormat_type::binary)
  {
      wfk="binary";
  }
  controlDict["writeFormat"]=wfk;

  controlDict["writePrecision"]=8;
  if (OFversion()>=600)
  {
    controlDict["writeCompression"]="on";
  }
  else
  {
    controlDict["writeCompression"]="compressed";
  }
  controlDict["timeFormat"]="general";
  controlDict["timePrecision"]=6;
  controlDict["runTimeModifiable"]=true;
  controlDict.getList("libs");
  controlDict.subDict("functions");

  controlDict.getList("libs").insertNoDuplicate( "\"libwriteData.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"libconsistentCurveSampleSet.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalLimitedSnGrad.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"libnumericsFunctionObjects.so\"" );

  {
    OFDictData::dict wonow;
    wonow["type"]="writeData";
    wonow["fileName"]="\"wnow\"";
    wonow["fileNameAbort"]="\"wnowandstop\"";
    wonow["outputControl"]="timeStep";
    wonow["outputInterval"]=1;
    controlDict.subDict("functions")["writeData"]=wonow;
  }
  {
    OFDictData::dict fqmc;
    fqmc["type"]="faceQualityMarker";
    fqmc["lowerNonOrthThreshold"]=35.0;
    fqmc["upperNonOrthThreshold"]=60.0;
    controlDict.subDict("functions")["faceQualityMarker"]=fqmc;
  }

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  fvSolution.subDict("relaxationFactors");

  // potentialFoam config
  solvers["Phi"]=OFcase().stdSymmSolverSetup(1e-7, 0.01);

  OFDictData::dict& PF=fvSolution.subDict("potentialFlow");
  PF["nNonOrthogonalCorrectors"]=10;
  PF["pRefCell"]=0;
  PF["pRefValue"]=0.0;
  PF["PhiRefCell"]=0;
  PF["PhiRefValue"]=0.0;

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  fvSchemes.subDict("ddtSchemes");

  fvSchemes.subDict("gradSchemes");
  insertStandardGradientConfig(dictionaries);

  fvSchemes.subDict("divSchemes");

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear localLimited UBlendingFactor 1";

  // potentialFoam
  laplacian["laplacian(1,Phi)"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="localLimited UBlendingFactor 1";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired[pName_]="";

  if (OFversion()>=300)
  {
    OFDictData::dict& wd = fvSchemes.subDict("wallDist");
    wd["method"]="meshWave";
    OFDictData::dict& wd2 = fvSchemes.subDict("patchDist");
    wd2["method"]="meshWave";
  }

  if ( const auto* map = boost::get<Parameters::mapFieldsConfig_map_type>(&p_.mapFieldsConfig) )
  {
    OFDictData::dict& mfd=dictionaries.lookupDict("system/mapFieldsDict");

    OFDictData::list patchMapPairs;
    for (const auto& i: map->patchMap)
    {
      patchMapPairs.push_back(i.targetPatch);
      patchMapPairs.push_back(i.sourcePatch);
    }
    mfd["patchMap"]=patchMapPairs;

    OFDictData::list cuttingPatches;
    std::copy(map->cuttingPatches.begin(), map->cuttingPatches.end(), std::back_inserter(cuttingPatches));
    mfd["cuttingPatches"]=cuttingPatches;
  }


  decomposeParDict::addIntoDictionaries(dictionaries);
}



bool FVNumerics::isLES() const
{
  // check if LES required
  bool LES=false;

  try
  {
    const turbulenceModel* tm = this->OFcase().get<turbulenceModel>("turbulenceModel");
    if (tm)
    {
      LES=LES || (tm->minAccuracyRequirement() >= turbulenceModel::AC_LES);
    }
  }
  catch (...)
  {
    insight::Warning("Warning: unhandled exception during LES check!");
  }

  return LES;
}


bool FVNumerics::isGAMGOk() const
{
  bool GAMG_ok=true;
  if (OFversion()<170)
  {
   if (
       (OFcase().findElements<OverlapGGIBC>().size()>0)
       ||
       (OFcase().findElements<MixingPlaneGGIBC>().size()>0)
       )
     GAMG_ok=false;
  }
  return GAMG_ok;
}



void FVNumerics::setApplicationName(OFdicts& dictionaries, const std::string& appname) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=appname;
}



void FVNumerics::setRelaxationFactors
(
    OFdicts& dictionaries,
    const std::map<std::string, double>& eqnRelax,
    const std::map<std::string, double>& fieldRelax
) const
{
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  if (OFversion()<210)
  {
    for (const auto& r: eqnRelax)
      relax[r.first]=r.second;
    for (const auto& r: fieldRelax)
      relax[r.first]=r.second;
  }
  else
  {
    OFDictData::dict dfieldRelax, deqnRelax;
    for (const auto& r: eqnRelax)
      deqnRelax[r.first]=r.second;
    for (const auto& r: fieldRelax)
      dfieldRelax[r.first]=r.second;
    relax["fields"]=dfieldRelax;
    relax["equations"]=deqnRelax;
  }
}



std::string FVNumerics::lqGradSchemeIfPossible() const
{
  if ( (OFversion()<220) )
  {
    return "leastSquares";
  }
  else
  {
    if ( OFcase().hasCyclicBC() )
      return "Gauss linear";
    else
      return "pointCellsLeastSquares";
  }
}



void FVNumerics::insertStandardGradientConfig(OFdicts& dictionaries) const
{
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");

  std::string bgrads=lqGradSchemeIfPossible();
  grad["default"]="cellLimited "+bgrads+" 1";
  grad["grad("+pName_+")"]="cellLimited Gauss linear 1";

//  grad["limitedGrad"]="cellMDLimited "+bgrads+" 1";
//  grad["grad(omega)"]="cellLimited "+bgrads+" 1";
//  grad["grad(epsilon)"]="cellLimited "+bgrads+" 1";
//  grad["grad(k)"]="cellLimited "+bgrads+" 1";
//  grad["grad(nuTilda)"]="cellLimited "+bgrads+" 1";
}



std::string FVNumerics::gradNameOrScheme(OFdicts& dictionaries, const std::string& key) const
{
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  if (OFversion()>=220)
  {
    return key;
  }
  else
  {
    const auto &grads = fvSchemes.subDict("gradSchemes");

    if (grads.find(key)!=grads.end())
      return grads.getString(key);
    else
      return grads.getString("default");
  }
}



bool FVNumerics::isUnique() const
{
  return true;
}







defineType(potentialFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(potentialFoamNumerics);



potentialFoamNumerics::potentialFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0., 0., 0.}), volField ) );
}



void potentialFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  setApplicationName(dictionaries, "potentialFoam");


  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=isGAMGOk() ? OFcase().GAMGPCGSolverSetup(1e-7, 0.01):OFcase().stdSymmSolverSetup(1e-7, 0.01);


  // ============ setup fvSchemes ================================

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["default"]          = "none";
  div["div(phi,U)"]       = "bounded Gauss linear";
  div["div(div(phi,U))"]  = "Gauss linear";
}

bool potentialFoamNumerics::isCompressible() const
{
  return false;
}

ParameterSet potentialFoamNumerics::defaultParameters()
{
  return Parameters::makeDefault();
}






defineType(laplacianFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(laplacianFoamNumerics);

laplacianFoamNumerics::laplacianFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("T", FieldInfo(vectorField, 	dimTemperature, 		FieldValue({p_.Tinternal}), volField ) );
}


void laplacianFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  setApplicationName(dictionaries, "laplacianFoam");


  // ============ setup fvSolution ================================
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["T"]=isGAMGOk()?OFcase().GAMGPCGSolverSetup(1e-7, 0.0):OFcase().stdAsymmSolverSetup(1e-7, 0.0);


  // ============ setup fvSchemes ================================
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";


  // ============ setup controlDict ================================
  OFDictData::dict& tp=dictionaries.lookupDict("constant/transportProperties");
  tp["DT"]=p_.DT;
}


bool laplacianFoamNumerics::isCompressible() const
{
  return false;
}


ParameterSet laplacianFoamNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}







FaNumerics::FaNumerics(OpenFOAMCase& c, const ParameterSet& p)
: OpenFOAMCaseElement(c, "FaNumerics", p), p_(p)
{
}

void FaNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& faSolution=dictionaries.lookupDict("system/faSolution");
  faSolution.subDict("solvers");
  faSolution.subDict("relaxationFactors");

  OFDictData::dict& faSchemes=dictionaries.lookupDict("system/faSchemes");
  faSchemes.subDict("ddtSchemes");
  faSchemes.subDict("gradSchemes");
  faSchemes.subDict("divSchemes");
  faSchemes.subDict("laplacianSchemes");
  faSchemes.subDict("interpolationSchemes");
  faSchemes.subDict("snGradSchemes");
  faSchemes.subDict("fluxRequired");
}


bool FaNumerics::isUnique() const
{
  return true;
}




tetFemNumerics::tetFemNumerics(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "tetFemNumerics", ParameterSet())
{
}

void tetFemNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& tetFemSolution=dictionaries.lookupDict("system/tetFemSolution");
  tetFemSolution.subDict("solvers");
}

bool tetFemNumerics::isUnique() const
{
  return true;
}





defineType(MeshingNumerics);
addToOpenFOAMCaseElementFactoryTable(MeshingNumerics);

MeshingNumerics::MeshingNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: decomposeParDict(c, ps)
{
  rename("MeshingNumerics");
}


void MeshingNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  //  std::cerr<<"Addd FVN "<<p_.decompWeights<<std::endl;

  // setup structure of dictionaries
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="none";
  controlDict["deltaT"]=1;
  controlDict["startFrom"]="latestTime";
  controlDict["startTime"]=0.0;
  controlDict["stopAt"]="endTime";
  controlDict["endTime"]=1000;
  controlDict["writeControl"]="timeStep";
  controlDict["writeInterval"]=1;
  controlDict["purgeWrite"]=0;
  controlDict["writeFormat"]="ascii";
  controlDict["writePrecision"]=8;
  if (OFversion()>=600)
  {
    controlDict["writeCompression"]="on";
  }
  else
  {
    controlDict["writeCompression"]="compressed";
  }
  controlDict["timeFormat"]="general";
  controlDict["timePrecision"]=6;
  controlDict["runTimeModifiable"]=true;
  controlDict.getList("libs");
  controlDict.subDict("functions");


  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  fvSolution.subDict("solvers");
  fvSolution.subDict("relaxationFactors");

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  fvSchemes.subDict("ddtSchemes");
  fvSchemes.subDict("gradSchemes");
  fvSchemes.subDict("divSchemes");
  fvSchemes.subDict("laplacianSchemes");
  fvSchemes.subDict("interpolationSchemes");
  fvSchemes.subDict("snGradSchemes");
  fvSchemes.subDict("fluxRequired");

  if (OFversion()>=300)
  {
    OFDictData::dict& wd = fvSchemes.subDict("wallDist");
    wd["method"]="meshWave";
    OFDictData::dict& wd2 = fvSchemes.subDict("patchDist");
    wd2["method"]="meshWave";
  }

  decomposeParDict::addIntoDictionaries(dictionaries);
}


ParameterSet MeshingNumerics::defaultParameters()
{
  return Parameters::makeDefault();
}

}
