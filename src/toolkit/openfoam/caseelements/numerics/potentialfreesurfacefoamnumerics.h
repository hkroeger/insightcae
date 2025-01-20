#ifndef INSIGHT_POTENTIALFREESURFACEFOAMNUMERICS_H
#define INSIGHT_POTENTIALFREESURFACEFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/unsteadyincompressiblenumerics.h"
#include "potentialfreesurfacefoamnumerics__potentialFreeSurfaceFoamNumerics__Parameters_headers.h"

namespace insight {

class potentialFreeSurfaceFoamNumerics
    : public unsteadyIncompressibleNumerics
{

public:
#include "potentialfreesurfacefoamnumerics__potentialFreeSurfaceFoamNumerics__Parameters.h"


/*
PARAMETERSET>>> potentialFreeSurfaceFoamNumerics Parameters
inherits unsteadyIncompressibleNumerics::Parameters

createGetters
<<<PARAMETERSET
*/


public:
    declareType ( "potentialFreeSurfaceFoamNumerics" );
    potentialFreeSurfaceFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;

};


} // namespace insight

#endif // INSIGHT_POTENTIALFREESURFACEFOAMNUMERICS_H
