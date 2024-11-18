#ifndef FVOPTION_H
#define FVOPTION_H

#include "openfoam/caseelements/openfoamcaseelement.h"
#include "fvoption__cellSetFvOption__Parameters_headers.h"

namespace insight {

class fvOption
: public OpenFOAMCaseElement
{
public:
    declareType ( "fvOption" );
    fvOption(
        OpenFOAMCase& c,
        const std::string& name,
        const ParameterSet& ps );

    virtual void addIntoFvOptionDictionary(
        OFDictData::dict& fvOptionDict,
        OFdicts& dictionaries ) const =0;

    void addIntoDictionaries(OFdicts& dictionaries) const override;
    void addIntoCustomFvOptionDictionary(
        OFDictData::dict& fvOptionDict,
        OFdicts& dictionaries ) const;
};


class cellSetFvOption
    : public fvOption
{
public:
#include "fvoption__cellSetFvOption__Parameters.h"
/*
PARAMETERSET>>> cellSetFvOption Parameters

execution = selectablesubset {{
 everytime set {}
 timeLimited set {
  timeStart = double -1 "start time"
  duration = double 1e99 "time duration in which the option is active"
 }
}} everytime ""

<<<PARAMETERSET
*/

private:
    Parameters p_;

public:
    declareType ( "cellSetFvOption" );
    cellSetFvOption(
        OpenFOAMCase& c,
        const std::string& name,
        const ParameterSet& ps );

    void addIntoFvOptionDictionary(
        OFDictData::dict& fvOptionDict,
        OFdicts& dictionaries ) const override;

};

} // namespace insight

#endif // FVOPTION_H
