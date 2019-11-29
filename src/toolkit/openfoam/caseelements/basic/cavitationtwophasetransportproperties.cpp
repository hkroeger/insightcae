#include "cavitationtwophasetransportproperties.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


namespace phaseChangeModels
{

defineType(phaseChangeModel);
defineFactoryTable(phaseChangeModel, LIST( const ParameterSet& ps ), LIST( ps ) );
defineStaticFunctionTable(phaseChangeModel, defaultParameters, ParameterSet);

phaseChangeModel::~phaseChangeModel()
{
}


defineType(SchnerrSauer);
addToFactoryTable(phaseChangeModel, SchnerrSauer);
addToStaticFunctionTable(phaseChangeModel, SchnerrSauer, defaultParameters);

SchnerrSauer::SchnerrSauer(const ParameterSet& p)
: p_(p)
{
}

void SchnerrSauer::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
  transportProperties["phaseChangeTwoPhaseMixture"]="SchnerrSauer";

  OFDictData::dict& coeffs=transportProperties.subDict("SchnerrSauerCoeffs");
  coeffs["n"] = OFDictData::dimensionedData("n", OFDictData::dimension(0, -3), p_.n);
  coeffs["dNuc"] = OFDictData::dimensionedData("dNuc", OFDictData::dimension(0, 1), p_.dNuc);
  coeffs["Cc"] = OFDictData::dimensionedData("Cc", OFDictData::dimension(), p_.Cc);
  coeffs["Cv"] = OFDictData::dimensionedData("Cv", OFDictData::dimension(), p_.Cv);
}

}




defineType(cavitationTwoPhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(cavitationTwoPhaseTransportProperties);

cavitationTwoPhaseTransportProperties::cavitationTwoPhaseTransportProperties( OpenFOAMCase& c, const ParameterSet& ps )
: twoPhaseTransportProperties(c, ps),
  ps_(ps)
{
}

void cavitationTwoPhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  Parameters p(ps_);
  twoPhaseTransportProperties::addIntoDictionaries(dictionaries);
  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
  transportProperties["pSat"]=OFDictData::dimensionedData("pSat", OFDictData::dimension(1, -1, -2), p.psat);

  const SelectableSubsetParameter& msp = ps_.get<SelectableSubsetParameter>("model");
  phaseChangeModels::phaseChangeModel::lookup(msp.selection(), msp()) ->addIntoDictionaries(dictionaries);
}

void cavitationTwoPhaseTransportProperties::modifyDefaults(ParameterSet& ps)
{

    SelectableSubsetParameter& msp = ps.get<SelectableSubsetParameter>("model");
    for (phaseChangeModels::phaseChangeModel::FactoryTable::const_iterator i = phaseChangeModels::phaseChangeModel::factories_->begin();
        i != phaseChangeModels::phaseChangeModel::factories_->end(); i++)
    {
        ParameterSet defp = phaseChangeModels::phaseChangeModel::defaultParameters(i->first);
        msp.addItem( i->first, defp );
    }
    msp.selection() = phaseChangeModels::phaseChangeModel::factories_->begin()->first;

}


} // namespace insight
