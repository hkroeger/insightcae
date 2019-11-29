#ifndef INSIGHT_MINIMUMTIMESTEPLIMIT_H
#define INSIGHT_MINIMUMTIMESTEPLIMIT_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {

class minimumTimestepLimit
    : public OpenFOAMCaseElement
{

public:
#include "minimumtimesteplimit__minimumTimestepLimit__Parameters.h"
/*
PARAMETERSET>>> minimumTimestepLimit Parameters

minDT = double 1e-10 "Minimum time step size. If automatic time step adaption underruns this value, output is written and simulation is stopped."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "minimumTimestepLimit" );
    minimumTimestepLimit ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category();
};

} // namespace insight

#endif // INSIGHT_MINIMUMTIMESTEPLIMIT_H
