#include "thermodynamicmodel.h"

namespace insight
{



thermodynamicModel::thermodynamicModel(
    OpenFOAMCase& c, ParameterSetInput ip)
: OpenFOAMCaseElement(c, ip.forward<Parameters>())
{}



}
