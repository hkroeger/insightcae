#ifndef INSIGHT_MINIMUMTIMESTEPLIMIT_H
#define INSIGHT_MINIMUMTIMESTEPLIMIT_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "minimumtimesteplimit__minimumTimestepLimit__Parameters_headers.h"

namespace insight {

class minimumTimestepLimit
    : public OpenFOAMCaseElement
{

public:
#include "minimumtimesteplimit__minimumTimestepLimit__Parameters.h"
/*
PARAMETERSET>>> minimumTimestepLimit Parameters
inherits OpenFOAMCaseElement::Parameters

minDT = double 1e-10 "Minimum time step size. If automatic time step adaption underruns this value, output is written and simulation is stopped."

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "minimumTimestepLimit" );
    minimumTimestepLimit ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category();
};

} // namespace insight

#endif // INSIGHT_MINIMUMTIMESTEPLIMIT_H
