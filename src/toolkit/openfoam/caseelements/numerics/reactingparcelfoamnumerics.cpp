#include "reactingparcelfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(reactingParcelFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(reactingParcelFoamNumerics);


reactingParcelFoamNumerics::reactingParcelFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: reactingFoamNumerics(c, ps)
{
}


void reactingParcelFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  reactingFoamNumerics::addIntoDictionaries(dictionaries);
}


} // namespace insight
