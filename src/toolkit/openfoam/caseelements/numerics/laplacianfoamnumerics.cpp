#include "laplacianfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(laplacianFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(laplacianFoamNumerics);

laplacianFoamNumerics::laplacianFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature, 		FieldValue({p_.Tinternal}), volField ) );
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

  OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors;

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


} // namespace insight
