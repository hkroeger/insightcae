#ifndef INSIGHT_MESHINGNUMERICS_H
#define INSIGHT_MESHINGNUMERICS_H


#include "openfoam/caseelements/basic/decomposepardict.h"
#include "meshingnumerics__MeshingNumerics__Parameters_headers.h"

namespace insight {

class MeshingNumerics
    : public decomposeParDict
{
public:
#include "meshingnumerics__MeshingNumerics__Parameters.h"

/*
PARAMETERSET>>> MeshingNumerics Parameters

inherits decomposeParDict::Parameters

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "MeshingNumerics" );

    MeshingNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

};

} // namespace insight

#endif // INSIGHT_MESHINGNUMERICS_H
