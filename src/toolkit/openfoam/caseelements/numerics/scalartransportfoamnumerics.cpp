#include "scalartransportfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {

defineType(scalarTransportFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(scalarTransportFoamNumerics);

scalarTransportFoamNumerics::scalarTransportFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature, 	FieldValue({p_.Tinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({p_.Uinternal[0],p_.Uinternal[1],p_.Uinternal[2]}), volField ) );
}


void scalarTransportFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  setApplicationName(dictionaries, "scalarTransportFoam");


  // ============ setup fvSolution ================================
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["T"]=OFcase().stdAsymmSolverSetup(1e-7, 0.0);

  OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors;


  // ============ setup fvSchemes ================================
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  OFDictData::dict& div = fvSchemes.subDict("divSchemes");
  div["div(phi,T)"]="Gauss linearUpwind grad(T)";

  OFDictData::dict& ddt = fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["laplacian(DT,T)"]="Gauss linear limited 1";

  // ============ setup controlDict ================================
  OFDictData::dict& tp=dictionaries.lookupDict("constant/transportProperties");
  tp["DT"]=p_.DT;
}


bool scalarTransportFoamNumerics::isCompressible() const
{
  return false;
}

} // namespace insight
