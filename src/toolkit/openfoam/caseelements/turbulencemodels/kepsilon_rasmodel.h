#ifndef INSIGHT_KEPSILON_RASMODEL_H
#define INSIGHT_KEPSILON_RASMODEL_H

#include "kepsilonbase_rasmodel.h"

namespace insight {

class kEpsilon_RASModel
: public kEpsilonBase_RASModel
{
public:
  declareType("kEpsilon");
  kEpsilon_RASModel(OpenFOAMCase& ofc, ParameterSetInput ip = Parameters() );
};


class continuousGasKEpsilon_RASModel
    : public kEpsilonBase_RASModel
{
public:
    declareType("continuousGasKEpsilon");
    continuousGasKEpsilon_RASModel(OpenFOAMCase& ofc, ParameterSetInput ip = Parameters() );
};


class LaheyKEpsilon_RASModel
    : public kEpsilonBase_RASModel
{
public:
    declareType("LaheyKEpsilon");
    LaheyKEpsilon_RASModel(OpenFOAMCase& ofc, ParameterSetInput ip = Parameters() );
};

} // namespace insight

#endif // INSIGHT_KEPSILON_RASMODEL_H
