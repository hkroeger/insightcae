#ifndef INSIGHT_VOLUMEDRAG_H
#define INSIGHT_VOLUMEDRAG_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "volumedrag__volumeDrag__Parameters_headers.h"

namespace insight {

class volumeDrag
    : public OpenFOAMCaseElement
{

public:
#include "volumedrag__volumeDrag__Parameters.h"
/*
PARAMETERSET>>> volumeDrag Parameters

name = string "volumeDrag" "Name of the volume drag element. Equals the name of the cell zone."
CD = vector (1 0 0) "Volume drag coefficient for each direction"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "volumeDrag" );
    volumeDrag ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_VOLUMEDRAG_H
