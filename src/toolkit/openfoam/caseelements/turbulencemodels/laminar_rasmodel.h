#ifndef INSIGHT_LAMINAR_RASMODEL_H
#define INSIGHT_LAMINAR_RASMODEL_H

#include "openfoam/caseelements/basic/rasmodel.h"

namespace insight {

class laminar_RASModel
: public RASModel
{
public:
  declareType("laminar");

  laminar_RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const override;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

} // namespace insight

#endif // INSIGHT_LAMINAR_RASMODEL_H
