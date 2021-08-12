#ifndef INSIGHT_DYNONEEQEDDY_LESMODEL_H
#define INSIGHT_DYNONEEQEDDY_LESMODEL_H

#include "openfoam/caseelements/basic/lesmodel.h"

namespace insight {

class dynOneEqEddy_LESModel
: public LESModel
{

public:
  declareType("dynOneEqEddy");

  dynOneEqEddy_LESModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

} // namespace insight

#endif // INSIGHT_DYNONEEQEDDY_LESMODEL_H
