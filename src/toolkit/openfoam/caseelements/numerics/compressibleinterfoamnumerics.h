#ifndef COMPRESSIBLEINTERFOAMNUMERICS_H
#define COMPRESSIBLEINTERFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/interfoamnumerics.h"

#include "compressibleinterfoamnumerics__compressibleInterFoamNumerics__Parameters_headers.h"

namespace insight {

class compressibleInterFoamNumerics
    : public interFoamNumerics
{

public:
#include "compressibleinterfoamnumerics__compressibleInterFoamNumerics__Parameters.h"
    /*
PARAMETERSET>>> compressibleInterFoamNumerics Parameters
inherits interFoamNumerics::Parameters
addTo_makeDefault { p.setDouble("pinternal", 1e5); }

Tinternal = double 300.0 "Internal pressure field value"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "compressibleInterFoamNumerics" );

    compressibleInterFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

}

#endif // COMPRESSIBLEINTERFOAMNUMERICS_H
