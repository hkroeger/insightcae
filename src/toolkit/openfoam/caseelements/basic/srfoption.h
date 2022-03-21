#ifndef INSIGHT_SRFOPTION_H
#define INSIGHT_SRFOPTION_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "srfoption__SRFoption__Parameters_headers.h"

namespace insight {

class SRFoption
: public OpenFOAMCaseElement
{

public:
#include "srfoption__SRFoption__Parameters.h"

/*
PARAMETERSET>>> SRFoption Parameters

origin = vector (0 0 0) "Center of the rotating motion"
axis = vector (0 0 1) "Axis of rotation"
rpm = double 1000.0 "Number of revolutions per minute"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType("SRFoption");
  SRFoption(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;

  static std::string category() { return "Body Force"; }
};

} // namespace insight

#endif // INSIGHT_SRFOPTION_H
