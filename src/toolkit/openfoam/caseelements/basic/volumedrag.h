#ifndef INSIGHT_VOLUMEDRAG_H
#define INSIGHT_VOLUMEDRAG_H

#include "openfoam/caseelements/basic/fvoption.h"

#include "volumedrag__volumeDrag__Parameters_headers.h"

namespace insight {

class volumeDrag
    : public cellSetFvOption
{

public:
#include "volumedrag__volumeDrag__Parameters.h"
/*
PARAMETERSET>>> volumeDrag Parameters
inherits cellSetFvOption::Parameters

name = string "volumeDrag" "Name of the volume drag element. Equals the name of the cell zone."
CD = vector (1 0 0) "Volume drag coefficient for each direction"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "volumeDrag" );
    volumeDrag ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoFvOptionDictionary(
        OFDictData::dict& fvOptions,
        OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_VOLUMEDRAG_H
