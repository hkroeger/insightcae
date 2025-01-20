#ifndef INSIGHT_PROVIDEFIELDS_H
#define INSIGHT_PROVIDEFIELDS_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "providefields__provideFields__Parameters_headers.h"

namespace insight {

class provideFields
    : public OpenFOAMCaseElement
{

public:
#include "providefields__provideFields__Parameters.h"
/*
PARAMETERSET>>> provideFields Parameters
inherits OpenFOAMCaseElement::Parameters

createScalarFields = array [ set {
  fieldName = string "" "Name of the field" *necessary
  dimensions = array [ int 0 "exponent of quantity" ] *4  "Exponents of Mass, Length, Time, Temperature" *necessary
  scalarValue = double 0.0 "Uniform field value"
 } ] *0 "Scalar fields to create"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "provideFields" );
    provideFields ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    virtual bool isUnique() const;

    static std::string category();
};

} // namespace insight

#endif // INSIGHT_PROVIDEFIELDS_H
