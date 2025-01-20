#ifndef INSIGHT_SMAGORINSKY_LESMODEL_H
#define INSIGHT_SMAGORINSKY_LESMODEL_H

#include "openfoam/caseelements/basic/lesmodel.h"

#include "smagorinsky_lesmodel__Smagorinsky_LESModel__Parameters_headers.h"

namespace insight {

class Smagorinsky_LESModel
: public LESModel
{
public:
#include "smagorinsky_lesmodel__Smagorinsky_LESModel__Parameters.h"
/*
PARAMETERSET>>> Smagorinsky_LESModel Parameters
inherits LESModel::Parameters

C = double 0.1 "Smagorinsky constant"

createGetters
<<<PARAMETERSET
*/

public:
  declareType("Smagorinsky");

  Smagorinsky_LESModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
};


} // namespace insight

#endif // INSIGHT_SMAGORINSKY_LESMODEL_H
