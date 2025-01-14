#ifndef INSIGHT_LEMOSHYBRID_RASMODEL_H
#define INSIGHT_LEMOSHYBRID_RASMODEL_H

#include "komegasst_rasmodel.h"

namespace insight {

class LEMOSHybrid_RASModel
: public kOmegaSST_RASModel
{

public:
  declareType("LEMOSHybrid");

  LEMOSHybrid_RASModel(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const override;

};

} // namespace insight

#endif // INSIGHT_LEMOSHYBRID_RASMODEL_H
