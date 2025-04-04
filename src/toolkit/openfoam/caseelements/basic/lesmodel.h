#ifndef INSIGHT_LESMODEL_H
#define INSIGHT_LESMODEL_H

#include "openfoam/caseelements/turbulencemodel.h"

namespace insight {

class LESModel
: public turbulenceModel
{

public:
  declareType("LESModel");

  LESModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  OFDictData::dict& modelPropsDict(OFdicts& dictionaries) const override;
  AccuracyRequirement minAccuracyRequirement() const override;
  bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const override;
};

} // namespace insight

#endif // INSIGHT_LESMODEL_H
