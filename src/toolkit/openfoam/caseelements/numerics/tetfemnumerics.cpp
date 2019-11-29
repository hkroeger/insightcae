#include "tetfemnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

tetFemNumerics::tetFemNumerics(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "tetFemNumerics", ParameterSet())
{
}

void tetFemNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& tetFemSolution=dictionaries.lookupDict("system/tetFemSolution");
  tetFemSolution.subDict("solvers");
}

bool tetFemNumerics::isUnique() const
{
  return true;
}

} // namespace insight
