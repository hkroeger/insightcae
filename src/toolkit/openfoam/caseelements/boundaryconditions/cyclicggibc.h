#ifndef INSIGHT_CYCLICGGIBC_H
#define INSIGHT_CYCLICGGIBC_H


#include "openfoam/caseelements/boundaryconditions/ggibcbase.h"

#include "cyclicggibc__CyclicGGIBC__Parameters_headers.h"

namespace insight {


class CyclicGGIBC
    : public GGIBCBase
{

public:
#include "cyclicggibc__CyclicGGIBC__Parameters.h"
/*
PARAMETERSET>>> CyclicGGIBC Parameters
inherits GGIBCBase::Parameters

separationOffset = vector (0 0 0) "Translational transformation from this patch to the opposite one."
rotationCentre = vector (0 0 0) "Origin of rotation axis"
rotationAxis = vector (0 0 1) "Direction of rotation axis"
rotationAngle = double 0.0 "[deg] Angle of rotation"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "CyclicGGIBC" );
    CyclicGGIBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
                  const ParameterSet&ps = Parameters::makeDefault() );
    void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const override;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;
};


} // namespace insight

#endif // INSIGHT_CYCLICGGIBC_H
