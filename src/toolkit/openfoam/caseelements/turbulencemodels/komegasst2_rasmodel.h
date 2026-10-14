#ifndef INSIGHT_KOMEGASST2_RASMODEL_H
#define INSIGHT_KOMEGASST2_RASMODEL_H

#include "komegasst_rasmodel.h"

namespace insight {

class kOmegaSST2_RASModel
: public kOmegaSST_RASModel
{
public:
  declareType("kOmegaSST2");

  kOmegaSST2_RASModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
//   virtual void addFields( OpenFOAMCase& c ) const;
  std::string RASModelName() const override;
  OFDictData::dict RASModelCoeffs() const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const override;

};

} // namespace insight

#endif // INSIGHT_KOMEGASST2_RASMODEL_H
