#ifndef INSIGHT_REACTINGPARCELFOAMNUMERICS_H
#define INSIGHT_REACTINGPARCELFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/reactingfoamnumerics.h"

namespace insight {

class reactingParcelFoamNumerics
    : public reactingFoamNumerics
{
public:
    declareType ( "reactingParcelFoamNumerics" );
    reactingParcelFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};

} // namespace insight

#endif // INSIGHT_REACTINGPARCELFOAMNUMERICS_H
