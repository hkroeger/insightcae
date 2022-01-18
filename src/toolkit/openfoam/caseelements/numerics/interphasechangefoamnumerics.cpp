#include "interphasechangefoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(interPhaseChangeFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(interPhaseChangeFoamNumerics);


interPhaseChangeFoamNumerics::interPhaseChangeFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: interFoamNumerics(c, ps),
  p_(ps)
{
}


void interPhaseChangeFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  interFoamNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=p_.solverName;

  // ============ setup controlDict ================================

//  controlDict["maxDeltaT"]=1.0;

//  controlDict["maxCo"]=0.4;
//  controlDict["maxAlphaCo"]=0.2;
//  if (p_.implicitPressureCorrection)
//  {
//    controlDict["maxCo"]=5;
//    controlDict["maxAlphaCo"]=2.5;
//  }

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");

  OFDictData::dict alphasol = stdMULESSolverSetup(p_.cAlpha, p_.icAlpha, 1e-12, 0.0, false);
  solvers["\"alpha.*\""]=alphasol;

}



} // namespace insight
