#include "transportmodel.h"


namespace insight {

transportModel::transportModel(OpenFOAMCase& c, const ParameterSet& ps)
: OpenFOAMCaseElement(c, "transportModel", ps)
{
}



bool transportModel::isUnique() const
{
  return true;
}

} // namespace insight
