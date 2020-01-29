#ifndef INSIGHT_PROVIDEFIELDS_H
#define INSIGHT_PROVIDEFIELDS_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {

class provideFields
    : public OpenFOAMCaseElement
{

public:
#include "providefields__provideFields__Parameters.h"
/*
PARAMETERSET>>> provideFields Parameters

createScalarFields = array [ set {
  fieldName = string "" "Name of the field" *necessary
  dimensions = array [ int 0 "exponent of quantity" ] *4  "Exponents of Mass, Length, Time, Temperature" *necessary
  scalarValue = double 0.0 "Uniform field value"
 } ] *0 "Scalar fields to create"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "provideFields" );
    provideFields ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    virtual bool isUnique() const;

    static std::string category();
};

} // namespace insight

#endif // INSIGHT_PROVIDEFIELDS_H
