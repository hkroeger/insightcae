#ifndef INSIGHT_KOMEGASST_RASMODEL_H
#define INSIGHT_KOMEGASST_RASMODEL_H

#include "openfoam/caseelements/basic/rasmodel.h"

#include "komegasst_rasmodel__kOmegaSST_RASModel__Parameters_headers.h"

namespace insight {

class kOmegaSST_RASModel
: public RASModel
{
public:
#include "komegasst_rasmodel__kOmegaSST_RASModel__Parameters.h"
/*
PARAMETERSET>>> kOmegaSST_RASModel Parameters
inherits RASModel::Parameters

freeSurfaceProductionDamping = selectablesubset {{

 none set { }

 enabled set {
   alphaFieldName = string "alpha.phase1" "Name of the volume fraction field"
 }

}} none "Option for selection of extra turbulence production damping close to the free surface in VOF simulations"

createGetters
<<<PARAMETERSET
*/

public:
  declareType("kOmegaSST");

  kOmegaSST_RASModel(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const override;
};

} // namespace insight

#endif // INSIGHT_KOMEGASST_RASMODEL_H
