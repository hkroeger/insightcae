#ifndef INSIGHT_POROUSZONE_H
#define INSIGHT_POROUSZONE_H

#include "openfoam/caseelements/openfoamcaseelement.h"


namespace insight {

class porousZone
    : public OpenFOAMCaseElement
{

public:
#include "porouszone__porousZone__Parameters.h"
/*
PARAMETERSET>>> porousZone Parameters

name = string "porosity" "Name of the porous cell zone. It needs to exist for this configuration to work."
d = vector (1 1 1) "Darcy coefficients for each direction"
f = vector (0 0 0) "Forchheimer coefficients for each direction"

direction_x = vector (1 0 0) "X direction of the porosity coordinate system"
direction_y = vector (0 1 0) "Y direction of the porosity coordinate system"
<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "porousZone" );
    porousZone ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};


} // namespace insight

#endif // INSIGHT_POROUSZONE_H
