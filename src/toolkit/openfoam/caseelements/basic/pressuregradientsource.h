#ifndef INSIGHT_PRESSUREGRADIENTSOURCE_H
#define INSIGHT_PRESSUREGRADIENTSOURCE_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "pressuregradientsource__PressureGradientSource__Parameters_headers.h"

namespace insight {

class PressureGradientSource
    : public OpenFOAMCaseElement
{


public:
#include "pressuregradientsource__PressureGradientSource__Parameters.h"
/*
PARAMETERSET>>> PressureGradientSource Parameters

Ubar = vector (0 0 0) "Average velocity"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "PressureGradientSource" );
    PressureGradientSource ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_PRESSUREGRADIENTSOURCE_H
