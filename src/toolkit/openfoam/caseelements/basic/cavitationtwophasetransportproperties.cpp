#include "cavitationtwophasetransportproperties.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


namespace phaseChangeModels
{




defineType(phaseChangeModel);
defineDynamicClass(phaseChangeModel);




phaseChangeModel::phaseChangeModel(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}

phaseChangeModel::~phaseChangeModel()
{}




defineType(SchnerrSauer);
addToFactoryTable(phaseChangeModel, SchnerrSauer);
addToStaticFunctionTable(phaseChangeModel, SchnerrSauer, defaultParameters);

SchnerrSauer::SchnerrSauer(ParameterSetInput ip)
: phaseChangeModel(ip.forward<Parameters>())
{}

void SchnerrSauer::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties =
          dictionaries.lookupDict("constant/transportProperties");
  transportProperties["phaseChangeTwoPhaseMixture"]="SchnerrSauer";

  OFDictData::dict& coeffs=transportProperties.subDict("SchnerrSauerCoeffs");
  coeffs["n"] = OFDictData::dimensionedData("n", OFDictData::dimension(0, -3), p().n);
  coeffs["dNuc"] = OFDictData::dimensionedData("dNuc", OFDictData::dimension(0, 1), p().dNuc);
  coeffs["Cc"] = OFDictData::dimensionedData("Cc", OFDictData::dimension(), p().Cc);
  coeffs["Cv"] = OFDictData::dimensionedData("Cv", OFDictData::dimension(), p().Cv);
}

}





defineType(cavitationTwoPhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(cavitationTwoPhaseTransportProperties);


cavitationTwoPhaseTransportProperties::cavitationTwoPhaseTransportProperties(
        OpenFOAMCase& c, ParameterSetInput ip )
: twoPhaseTransportProperties(c, ip.forward<Parameters>())
{}


void cavitationTwoPhaseTransportProperties::addIntoDictionaries(
        OFdicts& dictionaries
        ) const
{
  twoPhaseTransportProperties::addIntoDictionaries(dictionaries);

  OFDictData::dict& transportProperties =
          dictionaries.lookupDict("constant/transportProperties");
  transportProperties["pSat"]=
          OFDictData::dimensionedData("pSat", OFDictData::dimension(1, -1, -2), p().psat);

  p().model->addIntoDictionaries(dictionaries);
}



} // namespace insight
