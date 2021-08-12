#ifndef INSIGHT_REALIZABLEKEPSILON_RASMODEL_H
#define INSIGHT_REALIZABLEKEPSILON_RASMODEL_H

#include "kepsilonbase_rasmodel.h"

namespace insight {

class realizablekEpsilon_RASModel
: public kEpsilonBase_RASModel
{
public:
  declareType("realizableKE");
  realizablekEpsilon_RASModel(OpenFOAMCase& ofc, const ParameterSet& ps = ParameterSet());
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

} // namespace insight

#endif // INSIGHT_REALIZABLEKEPSILON_RASMODEL_H
