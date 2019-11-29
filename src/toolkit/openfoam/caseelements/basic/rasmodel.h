#ifndef INSIGHT_RASMODEL_H
#define INSIGHT_RASMODEL_H

#include "openfoam/caseelements/turbulencemodel.h"

namespace insight {

class RASModel
: public turbulenceModel
{

public:
  declareType("RASModel");

  RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  OFDictData::dict& modelPropsDict(OFdicts& dictionaries) const override;
  AccuracyRequirement minAccuracyRequirement() const override;
};

} // namespace insight

#endif // INSIGHT_RASMODEL_H
