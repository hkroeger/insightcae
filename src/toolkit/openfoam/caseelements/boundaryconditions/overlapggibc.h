#ifndef INSIGHT_OVERLAPGGIBC_H
#define INSIGHT_OVERLAPGGIBC_H


#include "openfoam/caseelements/boundaryconditions/ggibcbase.h"

#include "overlapggibc__OverlapGGIBC__Parameters_headers.h"

namespace insight {



class OverlapGGIBC
    : public GGIBCBase
{

public:
#include "overlapggibc__OverlapGGIBC__Parameters.h"
/*
PARAMETERSET>>> OverlapGGIBC Parameters
inherits GGIBCBase::Parameters

separationOffset = vector (0 0 0) "Translational transformation from this patch to the opposite one."
rotationAxis = vector (0 0 1) "Direction of rotation axis (only required, if foam-extend is used)"
nCopies = int 1 "number of copies (only required, if foam-extend is used)"
periodicPatch = string "" "lateral periodic patch, which determines the transform (only required, if openfoam.org versions are used)"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "OverlapGGIBC" );
    OverlapGGIBC (
        OpenFOAMCase& c, const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = Parameters() );

    void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const override;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;
};



} // namespace insight

#endif // INSIGHT_OVERLAPGGIBC_H
