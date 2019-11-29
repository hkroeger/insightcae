#include "turbulencemodel.h"

namespace insight {




defineType(turbulenceModel);
defineFactoryTable(turbulenceModel, LIST(OpenFOAMCase& ofc, const ParameterSet& ps), LIST(ofc, ps));




turbulenceModel::turbulenceModel(OpenFOAMCase& c, const ParameterSet& ps)
: OpenFOAMCaseElement(c, "turbulenceModel", ps)
{
}




} // namespace insight
