#include "fvoption.h"
#include "openfoam/ofdicts.h"

namespace insight {




defineType(fvOption);




fvOption::fvOption(
    OpenFOAMCase &c,
    const std::string& name,
    const ParameterSet &ps )
    : OpenFOAMCaseElement(c, name, ps)
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
    const std::string& name,
    const ParameterSet& ps )
: fvOption(c, name, ps),
    p_(ps)
{}


void cellSetFvOption::addIntoFvOptionDictionary(
    OFDictData::dict& fvOptions,
    OFdicts& dictionaries ) const
{
    auto& d = fvOptions.subDict(name());
    d["active"]=true;
    if (auto* tl =
        boost::get<Parameters::execution_timeLimited_type>(
            &p_.execution))
    {
        d["timeStart"]=tl->timeStart;
        d["duration"]=tl->duration;
    }
}




} // namespace insight
