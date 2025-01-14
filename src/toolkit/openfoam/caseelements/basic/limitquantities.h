#ifndef INSIGHT_LIMITQUANTITIES_H
#define INSIGHT_LIMITQUANTITIES_H

#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/caseelements/basic/cellsetoption_selection.h"

#include "limitquantities__limitQuantities__Parameters_headers.h"

namespace insight {

class limitQuantities
    : public OpenFOAMCaseElement
{

public:
#include "limitquantities__limitQuantities__Parameters.h"
/*
PARAMETERSET>>> limitQuantities Parameters
inherits OpenFOAMCaseElement::Parameters

name = string "limitQty" "Name prefix of the fvOptions"

cells = includedset "cellSetOption_Selection::Parameters" "selection of the cells, inside which the quantities are limited"

limitTemperature = selectablesubset {{
 none set {}
 limit set {
  min = double 0 "lower limit of temperature to enforce"
  max = double 1e10 "upper limit of temperature to enforce"
 }
}} none "Type of temperature limit"

limitVelocity = selectablesubset {{
 none set {}
 limit set {
  max = double 1e10 "upper limit of velocity magnitude"
 }
}} none "Type of temperature limit"

limitFields = array [ set {
 fieldName = string "p_rgh" "Name of field to limit"
 type = selection (scalar vector symmTensor tensor) scalar "Type of field"
 min = double -1e10 "Minimum value (magnitude for non-scalar fields)"
 max = double 1e10 "Maximum value (magnitude for non-scalar fields)"
} ] *0 "Fields to limit"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "limitQuantities" );
    limitQuantities ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Tweaks"; }
};

} // namespace insight

#endif // INSIGHT_LIMITQUANTITIES_H
