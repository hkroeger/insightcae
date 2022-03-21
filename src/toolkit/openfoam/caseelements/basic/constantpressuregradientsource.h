#ifndef INSIGHT_CONSTANTPRESSUREGRADIENTSOURCE_H
#define INSIGHT_CONSTANTPRESSUREGRADIENTSOURCE_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "constantpressuregradientsource__ConstantPressureGradientSource__Parameters_headers.h"

namespace insight {

class ConstantPressureGradientSource
    : public OpenFOAMCaseElement
{

public:
#include "constantpressuregradientsource__ConstantPressureGradientSource__Parameters.h"
/*
PARAMETERSET>>> ConstantPressureGradientSource Parameters

gradp = vector (0 0 0) "Constant pressure gradient"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "ConstantPressureGradientSource" );
    ConstantPressureGradientSource ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_CONSTANTPRESSUREGRADIENTSOURCE_H
