#ifndef INSIGHT_LRR_RASMODEL_H
#define INSIGHT_LRR_RASMODEL_H

#include "openfoam/caseelements/basic/rasmodel.h"

namespace insight {

class LRR_RASModel
: public RASModel
{
public:
  declareType("LRR");

  LRR_RASModel(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const override;
};

} // namespace insight

#endif // INSIGHT_LRR_RASMODEL_H
