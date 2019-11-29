#ifndef INSIGHT_LTSINTERFOAMNUMERICS_H
#define INSIGHT_LTSINTERFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/interfoamnumerics.h"

namespace insight {

class LTSInterFoamNumerics
    : public interFoamNumerics
{
public:
    declareType ( "LTSInterFoamNumerics" );
    LTSInterFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};

} // namespace insight

#endif // INSIGHT_LTSINTERFOAMNUMERICS_H
