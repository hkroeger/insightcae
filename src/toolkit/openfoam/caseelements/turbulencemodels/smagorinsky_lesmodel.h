#ifndef INSIGHT_SMAGORINSKY_LESMODEL_H
#define INSIGHT_SMAGORINSKY_LESMODEL_H

#include "openfoam/caseelements/basic/lesmodel.h"

namespace insight {

class Smagorinsky_LESModel
: public LESModel
{
public:
#include "smagorinsky_lesmodel__Smagorinsky_LESModel__Parameters.h"
/*
PARAMETERSET>>> Smagorinsky_LESModel Parameters

C = double 0.1 "Smagorinsky constant"

<<<PARAMETERSET
*/
protected:
    Parameters p_;

public:
  declareType("Smagorinsky");

  Smagorinsky_LESModel(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault());
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
};


} // namespace insight

#endif // INSIGHT_SMAGORINSKY_LESMODEL_H
