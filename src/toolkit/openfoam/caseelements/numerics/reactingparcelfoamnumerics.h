#ifndef INSIGHT_REACTINGPARCELFOAMNUMERICS_H
#define INSIGHT_REACTINGPARCELFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/reactingfoamnumerics.h"

namespace insight {

class reactingParcelFoamNumerics
    : public reactingFoamNumerics
{
public:
    declareType ( "reactingParcelFoamNumerics" );
    reactingParcelFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};

} // namespace insight

#endif // INSIGHT_REACTINGPARCELFOAMNUMERICS_H
