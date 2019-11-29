#ifndef INSIGHT_SOURCE_H
#define INSIGHT_SOURCE_H

#include "openfoam/caseelements/openfoamcaseelement.h"


namespace insight {

class source
: public OpenFOAMCaseElement
{

public:
#include "source__source__Parameters.h"
/*
PARAMETERSET>>> source Parameters

name = string "source" "Name of the volume drag element"
fieldName = string "T" "Name of the field to which this source shall contribute"
zoneName = string "" "Name of the zone with the cells which receive the source contribution"

volumeMode = selection (absolute specific) absolute
"Whether the value is specific or not. If absolute, the unit is that of the base field. If specific, it is that of the base field divided by $m^3$."

value = selectablesubset {{
 scalar set {
  value_const = double 0.0 "constant contribution"
  value_lin = double 0.0 "contribution propotional to base field value"
 }

 vector set {
  value_const = vector (0 0 0) "constant contribution"
  value_lin = vector (0 0 0) "contribution propotional to base field value"
 }

}} scalar "value of the source"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "source" );
    source ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_SOURCE_H
