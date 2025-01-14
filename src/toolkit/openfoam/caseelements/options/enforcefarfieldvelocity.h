#ifndef INSIGHT_ENFORCEFARFIELDVELOCITY_H
#define INSIGHT_ENFORCEFARFIELDVELOCITY_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "enforcefarfieldvelocity__EnforceFarFieldVelocity__Parameters_headers.h"

namespace insight {

class EnforceFarFieldVelocity
        : public OpenFOAMCaseElement
{
public:

#include "enforcefarfieldvelocity__EnforceFarFieldVelocity__Parameters.h"

/*
PARAMETERSET>>> EnforceFarFieldVelocity Parameters
inherits OpenFOAMCaseElement::Parameters

farFieldPatches = array [
 string "" "patch name or regular expression (if set in quotes)"
] *0 "selection of patches which make up the far field boundary"

farFieldVelocityFieldName = string "zero" "name of the far field value. Reserved name 'zero' enforces a zero field value."

transitionDistance = double 1 "distance within which the value is enforces"

createGetters
<<<PARAMETERSET
*/

public:
    declareType ( "EnforceFarFieldVelocity" );

    EnforceFarFieldVelocity(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    static std::string category() { return "Tweaks"; }
};

} // namespace insight

#endif // INSIGHT_ENFORCEFARFIELDVELOCITY_H
