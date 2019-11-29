#include "singlephasetransportmodel.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(singlePhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(singlePhaseTransportProperties);

singlePhaseTransportProperties::singlePhaseTransportProperties( OpenFOAMCase& c, const ParameterSet& ps )
: transportModel(c, ps),
  p_(ps)
{
}

void singlePhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
  transportProperties["transportModel"]="Newtonian";
  transportProperties["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu);
}


} // namespace insight
