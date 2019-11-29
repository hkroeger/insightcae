#include "ltsinterfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(LTSInterFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(LTSInterFoamNumerics);


LTSInterFoamNumerics::LTSInterFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: interFoamNumerics(c, ps)
{
}


void LTSInterFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  interFoamNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  if (OFversion()<300)
  {
    controlDict["application"]="LTSInterFoam";
  }

  //double maxCo=10.0, maxAlphaCo=5.0;
  double maxCo=10.0, maxAlphaCo=2.0;
  bool momentumPredictor=false;

  controlDict["maxAlphaCo"]=maxAlphaCo;
  controlDict["maxCo"]=maxCo;

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");

  if (OFversion()>=230)
    solvers["\"alpha.*\""]=stdMULESSolverSetup(0.25, 0.1, 1e-8, 0.0, true);


  std::string solutionScheme("PIMPLE");
  OFDictData::dict& SOL=fvSolution.subDict(solutionScheme);
  SOL["momentumPredictor"]=momentumPredictor;
  SOL["nCorrectors"]=2;
  SOL["nNonOrthogonalCorrectors"]=1;
  SOL["nAlphaCorr"]=1;
  SOL["nAlphaSubCycles"]=1;
  SOL["cAlpha"]=p_.cAlpha;
  SOL["maxAlphaCo"]=maxAlphaCo;
  SOL["maxCo"]=maxCo;
  SOL["rDeltaTSmoothingCoeff"]=0.05;
  SOL["rDeltaTDampingCoeff"]=0.5;
  SOL["nAlphaSweepIter"]=0;
  SOL["nAlphaSpreadIter"]=0;
  SOL["maxDeltaT"]=1;

  // ============ setup fvSchemes ================================

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  if (OFversion()<300)
  {
    ddt["default"]="localEuler rDeltaT";
  }
  else
  {
    ddt["default"]="localEuler";
  }

//   OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
}



} // namespace insight
