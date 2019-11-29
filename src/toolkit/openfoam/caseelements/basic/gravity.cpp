#include "gravity.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(gravity);
addToOpenFOAMCaseElementFactoryTable(gravity);

gravity::gravity( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "gravity", ps),
  p_(ps)
{
}

void gravity::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& g=dictionaries.lookupDict("constant/g");
  g["dimensions"]="[0 1 -2 0 0 0 0]";
  OFDictData::list gv;
  for (size_t i=0; i<3; i++) gv.push_back(p_.g(i));
  g["value"]=gv;
}


std::string gravity::category()
{
  return "Body Force";
}

bool gravity::isUnique() const
{
  return true;
}


} // namespace insight
