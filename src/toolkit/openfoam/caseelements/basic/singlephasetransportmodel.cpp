#include "singlephasetransportmodel.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {




defineType(singlePhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(singlePhaseTransportProperties);




singlePhaseTransportProperties::singlePhaseTransportProperties(
    OpenFOAMCase& c, ParameterSetInput ip )
: transportModel(c, ip.forward<Parameters>())
{}




void singlePhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
  transportProperties["transportModel"]="Newtonian";
  transportProperties["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p().nu);
}


defineType(boussinesqSinglePhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(boussinesqSinglePhaseTransportProperties);




boussinesqSinglePhaseTransportProperties::boussinesqSinglePhaseTransportProperties
(
    OpenFOAMCase& c,
    ParameterSetInput ip
)
  : singlePhaseTransportProperties(c, ip.forward<Parameters>())
{}



void boussinesqSinglePhaseTransportProperties::addIntoDictionaries ( OFdicts& dictionaries ) const
{
  singlePhaseTransportProperties::addIntoDictionaries(dictionaries);

  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
  transportProperties["beta"]=p().beta;
  transportProperties["TRef"]=p().TRef;
  transportProperties["Pr"]=p().Pr;
  transportProperties["Prt"]=p().Prt;
}

double boussinesqSinglePhaseTransportProperties::density(double T) const
{
    return 1.-p().beta*(T-p().TRef);
}




} // namespace insight
