#include "potentialfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {

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


} // namespace insight
