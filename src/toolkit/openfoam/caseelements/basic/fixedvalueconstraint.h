#ifndef INSIGHT_FIXEDVALUECONSTRAINT_H
#define INSIGHT_FIXEDVALUECONSTRAINT_H

#include "openfoam/caseelements/basic/fvoption.h"

#include "fixedvalueconstraint__fixedValueConstraint__Parameters_headers.h"

namespace insight {

class fixedValueConstraint
: public cellSetFvOption
{

public:
#include "fixedvalueconstraint__fixedValueConstraint__Parameters.h"
/*
PARAMETERSET>>> fixedValueConstraint Parameters
inherits cellSetFvOption::Parameters

name = string "fixedValueConstraint" "Name of the volume drag element"
fieldName = string "U" "Name of the field which shall be constrained"
zoneName = string "" "Name of the zone with the cells whose value shall be constrained"

value = selectablesubset {{
 scalar set {
  value = double 0.0 "value inside zone"
 }

 vector set {
  value = vector (0 0 0) "vector value inside zone"
 }

}} vector "value inside zone"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "fixedValueConstraint" );
    fixedValueConstraint ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoFvOptionDictionary(
        OFDictData::dict& fvOptions,
        OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_FIXEDVALUECONSTRAINT_H
