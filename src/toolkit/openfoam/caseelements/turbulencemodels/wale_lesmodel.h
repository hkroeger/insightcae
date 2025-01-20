#ifndef INSIGHT_WALE_LESMODEL_H
#define INSIGHT_WALE_LESMODEL_H

#include "openfoam/caseelements/basic/lesmodel.h"

namespace insight {

class WALE_LESModel
: public LESModel
{
public:
  declareType("WALE");

  WALE_LESModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
};

} // namespace insight

#endif // INSIGHT_WALE_LESMODEL_H
