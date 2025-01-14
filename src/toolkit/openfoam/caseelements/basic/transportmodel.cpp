#include "transportmodel.h"


namespace insight {

transportModel::transportModel(OpenFOAMCase& c, ParameterSetInput ip)
    : OpenFOAMCaseElement(c, ip.forward<Parameters>()/*.set_name("transportModel")*/)
{
}



bool transportModel::isUnique() const
{
  return true;
}

} // namespace insight
