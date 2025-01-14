#ifndef INSIGHT_PRESSUREGRADIENTSOURCE_H
#define INSIGHT_PRESSUREGRADIENTSOURCE_H

#include "openfoam/caseelements/basic/fvoption.h"

#include "pressuregradientsource__PressureGradientSource__Parameters_headers.h"

namespace insight {

class PressureGradientSource
    : public cellSetFvOption
{


public:
#include "pressuregradientsource__PressureGradientSource__Parameters.h"
/*
PARAMETERSET>>> PressureGradientSource Parameters
inherits cellSetFvOption::Parameters

Ubar = vector (0 0 0) "Average velocity"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "PressureGradientSource" );
    PressureGradientSource ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoFvOptionDictionary(
        OFDictData::dict& fvOptions,
        OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_PRESSUREGRADIENTSOURCE_H
