#include "fanumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {

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


} // namespace insight
