#include "fsidisplacementextrapolationnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {



FSIDisplacementExtrapolationNumerics::FSIDisplacementExtrapolationNumerics(
    OpenFOAMCase& c, ParameterSetInput ip )
: FaNumerics(c, ip.forward<Parameters>())
{
  //c.addField("displacement", FieldInfo(vectorField, 	dimLength, 	list_of(0.0)(0.0)(0.0), volField ) );
}

void FSIDisplacementExtrapolationNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FaNumerics::addIntoDictionaries(dictionaries);

  // ============ setup faSolution ================================

  OFDictData::dict& faSolution=dictionaries.lookupDict("system/faSolution");

  OFDictData::dict& solvers=faSolution.subDict("solvers");
  solvers["displacement"]=OFcase().stdSymmSolverSetup(1e-7, 0.01);


  // ============ setup faSchemes ================================

  OFDictData::dict& faSchemes=dictionaries.lookupDict("system/faSchemes");

  OFDictData::dict& grad=faSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";

  OFDictData::dict& div=faSchemes.subDict("divSchemes");
  div["default"]="Gauss linear";

  OFDictData::dict& laplacian=faSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=faSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=faSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

}



} // namespace insight
