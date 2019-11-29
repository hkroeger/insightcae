#ifndef INSIGHT_TRANSPORTMODEL_H
#define INSIGHT_TRANSPORTMODEL_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {

class transportModel
: public OpenFOAMCaseElement
{
public:
  transportModel(OpenFOAMCase& c, const ParameterSet& ps);
  virtual bool isUnique() const;
};

} // namespace insight

#endif // INSIGHT_TRANSPORTMODEL_H
