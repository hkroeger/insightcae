#include "twophasetransportproperties.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(twoPhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(twoPhaseTransportProperties);


twoPhaseTransportProperties::twoPhaseTransportProperties( OpenFOAMCase& c, const ParameterSet& ps )
: transportModel(c, ps),
  p_(ps)
{
}

void twoPhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");

  if (OFversion()<230)
  {
    OFDictData::dict& twoPhase=transportProperties.subDict("twoPhase");
    twoPhase["transportModel"]="twoPhase";
    twoPhase["phase1"]="phase1";
    twoPhase["phase2"]="phase2";
  } else
  {
    OFDictData::list& pl=transportProperties.getList("phases");
    pl.push_back("phase1");
    pl.push_back("phase2");
  }

  OFDictData::dict& phase1=transportProperties.subDict("phase1");
  phase1["transportModel"]="Newtonian";
  phase1["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu1);
  phase1["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p_.rho1);

  OFDictData::dict& phase2=transportProperties.subDict("phase2");
  phase2["transportModel"]="Newtonian";
  phase2["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu2);
  phase2["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p_.rho2);

  transportProperties["sigma"]=OFDictData::dimensionedData("sigma", OFDictData::dimension(1, 0, -2), p_.sigma);

}

} // namespace insight
