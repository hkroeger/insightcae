#ifndef INSIGHT_CONSTANTPRESSUREGRADIENTSOURCE_H
#define INSIGHT_CONSTANTPRESSUREGRADIENTSOURCE_H

#include "openfoam/caseelements/basic/fvoption.h"

#include "constantpressuregradientsource__ConstantPressureGradientSource__Parameters_headers.h"

namespace insight {

class ConstantPressureGradientSource
    : public cellSetFvOption
{

public:
#include "constantpressuregradientsource__ConstantPressureGradientSource__Parameters.h"
/*
PARAMETERSET>>> ConstantPressureGradientSource Parameters
inherits cellSetFvOption::Parameters

gradp = vector (0 0 0) "Constant pressure gradient"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "ConstantPressureGradientSource" );
    ConstantPressureGradientSource (
        OpenFOAMCase& c,
        ParameterSetInput ip = Parameters() );

    void addIntoFvOptionDictionary(
        OFDictData::dict& fod,
        OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_CONSTANTPRESSUREGRADIENTSOURCE_H
