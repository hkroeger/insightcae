#include "reactingparcelfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(reactingParcelFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(reactingParcelFoamNumerics);


reactingParcelFoamNumerics::reactingParcelFoamNumerics(OpenFOAMCase& c, ParameterSetInput ip)
: reactingFoamNumerics(c, ip.forward<Parameters>())
{
}


void reactingParcelFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  reactingFoamNumerics::addIntoDictionaries(dictionaries);
}


} // namespace insight
