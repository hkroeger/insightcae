#include "twophasetransportproperties.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamdict.h"

#include "openfoam/caseelements/numerics/interfoamnumerics.h"

namespace insight {




defineType(twoPhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(twoPhaseTransportProperties);




twoPhaseTransportProperties::twoPhaseTransportProperties(
    OpenFOAMCase& c, ParameterSetInput ip )
: transportModel(c, ip.forward<Parameters>())
{}




void twoPhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");

  auto& ifn = OFcase().findUniqueElement<interFoamNumerics>();
  auto phases = ifn.phaseNames();

  if (OFversion()<230)
  {
    OFDictData::dict& twoPhase=transportProperties.subDict("twoPhase");
    twoPhase["transportModel"]="twoPhase";
    twoPhase["phase1"]=phases.first;
    twoPhase["phase2"]=phases.second;
  } else
  {
    OFDictData::list& pl=transportProperties.getList("phases");
    pl.push_back(phases.first);
    pl.push_back(phases.second);
  }

  OFDictData::dict& phase1=transportProperties.subDict(phases.first);
  phase1["transportModel"]="Newtonian";
  phase1["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p().nu1);
  phase1["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p().rho1);

  OFDictData::dict& phase2=transportProperties.subDict(phases.second);
  phase2["transportModel"]="Newtonian";
  phase2["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p().nu2);
  phase2["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p().rho2);

  transportProperties["sigma"]=
          OFDictData::dimensionedData(
          "sigma",
          OFDictData::dimension(1, 0, -2),
          p().sigma
      );

}

} // namespace insight
