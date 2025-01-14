#include "fvnumerics.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

#include "openfoam/caseelements/turbulencemodel.h"
#include "openfoam/caseelements/boundaryconditions/overlapggibc.h"
#include "openfoam/caseelements/boundaryconditions/mixingplaneggibc.h"

namespace insight {

defineType(FVNumerics);



FVNumerics::FVNumerics(OpenFOAMCase& c, ParameterSetInput ip, const std::string& pName)
: decomposeParDict(c, ip.forward<Parameters>()),
  pName_(pName)
{
  // rename("FVNumerics");
}



void FVNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
//  std::cerr<<"Addd FVN "<<p_.decompWeights<<std::endl;
  // setup structure of dictionaries
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["deltaT"]=p().deltaT;
  controlDict["maxCo"]=0.5;
  controlDict["startFrom"]="latestTime";
  controlDict["startTime"]=0.0;
  controlDict["stopAt"]="endTime";
  controlDict["endTime"]=p().endTime;

  std::string wfc("UNDEFINED");
  if (p().writeControl == Parameters::writeControl_type::adjustableRunTime)
  {
      wfc="adjustableRunTime";
  }
  else if (p().writeControl == Parameters::writeControl_type::clockTime)
  {
      wfc="clockTime";
  }
  else if (p().writeControl == Parameters::writeControl_type::cpuTime)
  {
      wfc="cpuTime";
  }
  else if (p().writeControl == Parameters::writeControl_type::runTime)
  {
      wfc="runTime";
  }
  else if (p().writeControl == Parameters::writeControl_type::timeStep)
  {
      wfc="timeStep";
  }

  controlDict["writeControl"]=wfc;


  if (
      p().writeControl == Parameters::writeControl_type::runTime
      ||
      p().writeControl == Parameters::writeControl_type::adjustableRunTime
      ||
      ( p().writeControl == Parameters::writeControl_type::timeStep && p().timeStep==1. )
      )
  {
    controlDict["writeInterval"]=std::min(p().writeInterval, p().endTime);
  }
  else
  {
    controlDict["writeInterval"]=p().writeInterval;
  }

  controlDict["purgeWrite"]=p().purgeWrite;

  std::string wfk("UNDEFINED");
  if (p().writeFormat == Parameters::writeFormat_type::ascii)
  {
      wfk="ascii";
  }
  else if (p().writeFormat == Parameters::writeFormat_type::binary)
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

  controlDict.getList("libs").insertNoDuplicate( "\"libconsistentCurveSampleSet.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalLimitedSnGrad.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"libnumericsFunctionObjects.so\"" );

  {
    OFDictData::dict wonow;
    wonow["type"]="writeData";
    wonow["fileName"]="\"wnow\"";
    wonow["fileNameAbort"]="\"wnowandstop\"";
    controlDict.subDict("functions")["writeData"]=wonow;
    controlDict.getList("libs").insertNoDuplicate( "\"libwriteData.so\"" );
  }

  if (const auto* rwp =
          boost::get<Parameters::restartWrite_clockTime_type>(&p().restartWrite))
  {
      OFDictData::dict rw;
      rw["type"]="restartWrite";
      rw["clockTimeInterval"]=rwp->clockTimeInterval;
      rw["nKeep"]=rwp->nKeep;
      controlDict.subDict("functions")["restartWrite"]=rw;
      controlDict.getList("libs").insertNoDuplicate( "\"librestartWrite.so\"" );
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
  if (OFversion()<170)
  {
    laplacian["laplacian(1,p)"]="Gauss linear limited 0.66";
  }
  else
  {
    laplacian["laplacian(1,Phi)"]="Gauss linear limited 0.66";
  }

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

  {
    OFDictData::dict& mfd=dictionaries.lookupDict("system/mapFieldsDict");

    OFDictData::list patchMapPairs;
    for (const auto& i: p().mapFields.patchMap)
    {
      patchMapPairs.push_back(i.targetPatch);
      patchMapPairs.push_back(i.sourcePatch);
    }
    mfd["patchMap"]=patchMapPairs;

    OFDictData::list cuttingPatches;
    std::copy(p().mapFields.cuttingPatches.begin(),
              p().mapFields.cuttingPatches.end(),
              std::back_inserter(cuttingPatches));
    mfd["cuttingPatches"]=cuttingPatches;
  }


  decomposeParDict::addIntoDictionaries(dictionaries);
}


// const FVNumerics::Parameters &FVNumerics::parameters() const
// {
//   return p_;
// }


// FVNumerics::Parameters &FVNumerics::parametersRef()
// {
//     return p_;
// }



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


} // namespace insight
