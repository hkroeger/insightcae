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


defineType(boussinesqSinglePhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(boussinesqSinglePhaseTransportProperties);




boussinesqSinglePhaseTransportProperties::boussinesqSinglePhaseTransportProperties
(
    OpenFOAMCase& c,
    const ParameterSet& ps
)
  : singlePhaseTransportProperties(c, ps),
    p_(ps)
{}



void boussinesqSinglePhaseTransportProperties::addIntoDictionaries ( OFdicts& dictionaries ) const
{
  singlePhaseTransportProperties::addIntoDictionaries(dictionaries);

  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
  transportProperties["beta"]=p_.beta;
  transportProperties["TRef"]=p_.TRef;
  transportProperties["Pr"]=p_.Pr;
  transportProperties["Prt"]=p_.Prt;
}

double boussinesqSinglePhaseTransportProperties::density(double T) const
{
    return 1.-p_.beta*(T-p_.TRef);
}




} // namespace insight
