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

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "potentialFreeSurfaceFoamNumerics" );
    potentialFreeSurfaceFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;

};


} // namespace insight

#endif // INSIGHT_POTENTIALFREESURFACEFOAMNUMERICS_H
