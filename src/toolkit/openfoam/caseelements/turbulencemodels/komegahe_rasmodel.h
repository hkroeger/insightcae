#ifndef INSIGHT_KOMEGAHE_RASMODEL_H
#define INSIGHT_KOMEGAHE_RASMODEL_H

#include "komegasst_rasmodel.h"

namespace insight {

class kOmegaHe_RASModel
: public kOmegaSST_RASModel
{
public:
  declareType("kOmegaHe");

  kOmegaHe_RASModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const override;
};

} // namespace insight

#endif // INSIGHT_KOMEGAHE_RASMODEL_H
