#ifndef INSIGHT_FSIDISPLACEMENTEXTRAPOLATIONNUMERICS_H
#define INSIGHT_FSIDISPLACEMENTEXTRAPOLATIONNUMERICS_H

#include "openfoam/caseelements/numerics/fanumerics.h"
#include "fsidisplacementextrapolationnumerics__FSIDisplacementExtrapolationNumerics__Parameters_headers.h"

namespace insight {

class FSIDisplacementExtrapolationNumerics
: public FaNumerics
{
public:
#include "fsidisplacementextrapolationnumerics__FSIDisplacementExtrapolationNumerics__Parameters.h"

/*
PARAMETERSET>>> FSIDisplacementExtrapolationNumerics Parameters
inherits FaNumerics::Parameters


<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  FSIDisplacementExtrapolationNumerics( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;
};

} // namespace insight

#endif // INSIGHT_FSIDISPLACEMENTEXTRAPOLATIONNUMERICS_H
