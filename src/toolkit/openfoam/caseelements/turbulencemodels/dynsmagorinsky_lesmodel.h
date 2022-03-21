#ifndef INSIGHT_DYNSMAGORINSKY_LESMODEL_H
#define INSIGHT_DYNSMAGORINSKY_LESMODEL_H

#include "openfoam/caseelements/basic/lesmodel.h"

namespace insight {

class dynSmagorinsky_LESModel
: public LESModel
{
public:
  declareType("dynSmagorinsky");

  dynSmagorinsky_LESModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};


} // namespace insight

#endif // INSIGHT_DYNSMAGORINSKY_LESMODEL_H
