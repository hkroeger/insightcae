#include "fvoption.h"
#include "openfoam/ofdicts.h"

namespace insight {

defineType(fvOption);

fvOption::fvOption(
    OpenFOAMCase &c,
    ParameterSetInput ip )
    : OpenFOAMCaseElement(c, ip.forward<Parameters>())
{}


void fvOption::addIntoDictionaries(OFdicts& dictionaries) const
{
    OFDictData::dict& fvOptions =
        dictionaries.lookupDict("system/fvOptions");
    addIntoFvOptionDictionary(fvOptions, dictionaries);
}

void fvOption::addIntoCustomFvOptionDictionary(
    OFDictData::dict& fvOptionDict,
    OFdicts& dictionaries ) const
{
    addIntoFvOptionDictionary(fvOptionDict, dictionaries);
}


defineType ( cellSetFvOption );

cellSetFvOption::cellSetFvOption(
    OpenFOAMCase& c,
    ParameterSetInput ip )
    : fvOption(c, ip.forward<Parameters>())
{}

void cellSetFvOption::addIntoFvOptionDictionary(
    OFDictData::dict& fvOptions,
    OFdicts& dictionaries ) const
{
    auto& d = fvOptions.subDict(name());
    d["active"]=true;
    if (auto* tl =
        boost::get<Parameters::execution_timeLimited_type>(
            &p().execution))
    {
        d["timeStart"]=tl->timeStart;
        d["duration"]=tl->duration;
    }
}

} // namespace insight
