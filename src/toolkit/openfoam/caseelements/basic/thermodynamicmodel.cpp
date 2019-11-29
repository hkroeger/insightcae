#include "thermodynamicmodel.h"

namespace insight
{



thermodynamicModel::thermodynamicModel(OpenFOAMCase& c, const ParameterSet& ps)
: OpenFOAMCaseElement(c, "thermodynamicModel", ps)
{
}



}
