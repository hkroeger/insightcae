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

createGetters
<<<PARAMETERSET
*/

public:
    declareType ( "MeshingNumerics" );

    MeshingNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

};

} // namespace insight

#endif // INSIGHT_MESHINGNUMERICS_H
