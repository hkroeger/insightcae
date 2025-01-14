#ifndef INSIGHT_LAMINAR_RASMODEL_H
#define INSIGHT_LAMINAR_RASMODEL_H

#include "openfoam/caseelements/basic/rasmodel.h"

namespace insight {

class laminar_RASModel
: public RASModel
{
public:
  declareType("laminar");

  laminar_RASModel(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const override;

};

} // namespace insight

#endif // INSIGHT_LAMINAR_RASMODEL_H
