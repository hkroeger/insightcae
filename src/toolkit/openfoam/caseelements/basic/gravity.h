#ifndef INSIGHT_GRAVITY_H
#define INSIGHT_GRAVITY_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {

class gravity
    : public OpenFOAMCaseElement
{

public:
#include "gravity__gravity__Parameters.h"
/*
PARAMETERSET>>> gravity Parameters

description
"This case elements adds the information about the gravity acceleration vector to the OpenFOAM case."

g = vector (0 0 -9.81) "[m/s^2] Gravity acceleration vector"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "gravity" );
    gravity ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    virtual bool isUnique() const;

    static std::string category();
};

} // namespace insight

#endif // INSIGHT_GRAVITY_H
