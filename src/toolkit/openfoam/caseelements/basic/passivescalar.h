#ifndef INSIGHT_PASSIVESCALAR_H
#define INSIGHT_PASSIVESCALAR_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {

class PassiveScalar
    : public OpenFOAMCaseElement
{

public:
#include "passivescalar__PassiveScalar__Parameters.h"
/*
PARAMETERSET>>> PassiveScalar Parameters

fieldname = string "F" "Name of the passive scalar field"
internal = double 0.0 "Default internal value"

underrelax = double 1.0 "Underrelaxation factor"

scheme = selectablesubset {{
 firstorder set {}
 secondorder set {
   bounded01 = bool true "If enabled, a 01 bounded scheme will be used for interpolation."
 }
}} secondorder "Accuracy/stability trade off"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "PassiveScalar" );
    PassiveScalar ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addFields( OpenFOAMCase& c ) const override;
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Numerics"; }
};

} // namespace insight

#endif // INSIGHT_PASSIVESCALAR_H
