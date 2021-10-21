#ifndef INSIGHT_COMPRESSIBLEINLETBC_H
#define INSIGHT_COMPRESSIBLEINLETBC_H

#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"

#include "compressibleinletbc__CompressibleInletBC__Parameters_headers.h"

namespace insight {

class CompressibleInletBC
    : public VelocityInletBC
{
public:
#include "compressibleinletbc__CompressibleInletBC__Parameters.h"
/*
PARAMETERSET>>> CompressibleInletBC Parameters
inherits VelocityInletBC::Parameters

pressure = double 1e5 "Static pressure at the inlet"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType ( "CompressibleInletBC" );

    CompressibleInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& p = Parameters::makeDefault()
    );
    virtual void setField_p ( OFDictData::dict& BC, OFdicts& dictionaries, bool isPrgh ) const;


};

} // namespace insight

#endif // INSIGHT_COMPRESSIBLEINLETBC_H
