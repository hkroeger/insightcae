#ifndef INSIGHT_SOURCE_H
#define INSIGHT_SOURCE_H

#include "openfoam/caseelements/basic/fvoption.h"

#include "source__source__Parameters_headers.h"

namespace insight {

class source
: public cellSetFvOption
{

public:
#include "source__source__Parameters.h"
/*
PARAMETERSET>>> source Parameters
inherits cellSetFvOption::Parameters

#name = string "source" "Name of the volume drag element"
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
  value_lin = double 0 "contribution propotional to base field value"
 }

}} scalar "value of the source"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "source" );
    source ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoFvOptionDictionary(
        OFDictData::dict& fvOptionDict,
        OFdicts& dictionaries) const override;

    static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_SOURCE_H
