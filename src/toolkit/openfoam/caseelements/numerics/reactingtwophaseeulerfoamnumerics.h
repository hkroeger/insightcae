#ifndef REACTINGTWOPHASEEULERFOAMNUMERICS_H
#define REACTINGTWOPHASEEULERFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"

#include "reactingtwophaseeulerfoamnumerics_pdl.h"


namespace insight {

class reactingTwoPhaseEulerFoamNumerics
: public FVNumerics
{
public:
#include REACTINGTWOPHASEEULERFOAMNUMERICS_PDL_reactingTwoPhaseEulerFoamNumerics
/*
PARAMETERSET>>>
inherits OpenFOAMCaseElement::Parameters

description
"This case elements is yet a stub"

createGetter
<<<PARAMETERSET
*/

public:
    declareType("reactingTwoPhaseEulerFoamNumerics");
    reactingTwoPhaseEulerFoamNumerics(OpenFOAMCase &c, ParameterSetInput ip = Parameters());
    void addIntoDictionaries(OFdicts &dictionaries) const override;

    bool isCompressible() const override;
};

} // namespace insight

#endif
