#ifndef INSIGHT_ONEEQEDDY_LESMODEL_H
#define INSIGHT_ONEEQEDDY_LESMODEL_H

#include "openfoam/caseelements/basic/lesmodel.h"

namespace insight {

class oneEqEddy_LESModel
: public LESModel
{

public:
  declareType("oneEqEddy");

  oneEqEddy_LESModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
};

} // namespace insight

#endif // INSIGHT_ONEEQEDDY_LESMODEL_H
