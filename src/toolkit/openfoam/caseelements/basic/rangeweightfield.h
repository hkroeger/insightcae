#ifndef RANGEWEIGHTFIELD_H
#define RANGEWEIGHTFIELD_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "rangeweightfield__rangeWeightField__Parameters_headers.h"

namespace insight {

class rangeWeightField
    : public OpenFOAMCaseElement
{

public:
#include "rangeweightfield__rangeWeightField__Parameters.h"
/*
PARAMETERSET>>> rangeWeightField Parameters

sourceFieldName = string "" "Source field name"
min = double 0 "minimum value"
max = double 1 "maximum value"
outputFieldName = string "" "Output field name"
multiplyFieldName = string "" "Optional name of field with which the output field is to be multiplied. No multiplication is done, if the field is empty."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "rangeWeightField" );
    rangeWeightField ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    virtual bool isUnique() const;

    static std::string category();
};

} // namespace insight


#endif // RANGEWEIGHTFIELD_H
