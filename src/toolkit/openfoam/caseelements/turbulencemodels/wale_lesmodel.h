#ifndef INSIGHT_WALE_LESMODEL_H
#define INSIGHT_WALE_LESMODEL_H

#include "openfoam/caseelements/basic/lesmodel.h"

namespace insight {

class WALE_LESModel
: public LESModel
{
public:
  declareType("WALE");

  WALE_LESModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

} // namespace insight

#endif // INSIGHT_WALE_LESMODEL_H
