#ifndef INSIGHT_MIXINGPLANEGGIBC_H
#define INSIGHT_MIXINGPLANEGGIBC_H

#include "openfoam/caseelements/boundaryconditions/ggibcbase.h"

#include "mixingplaneggibc__MixingPlaneGGIBC__Parameters_headers.h"

namespace insight {


class MixingPlaneGGIBC
    : public GGIBCBase
{

public:
#include "mixingplaneggibc__MixingPlaneGGIBC__Parameters.h"
/*
PARAMETERSET>>> MixingPlaneGGIBC Parameters
inherits GGIBCBase::Parameters

separationOffset = vector (0 0 0) "Translational transformation from this patch to the opposite one."
stackAxisOrientation = selection ( axial radial ) axial "In which direction are the ribbons on the intermediate patch stacked."
rotationCentre = vector (0 0 0) "Origin of rotation axis"
rotationAxis = vector (0 0 1) "Direction of rotation axis"

createGetter
<<<PARAMETERSET
*/


public:
    declareType ( "MixingPlaneGGIBC" );
    MixingPlaneGGIBC (
        OpenFOAMCase& c, const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = ParameterSetInput() );
    void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const override;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};



} // namespace insight

#endif // INSIGHT_MIXINGPLANEGGIBC_H
