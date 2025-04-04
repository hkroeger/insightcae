#include "kepsilon_rasmodel.h"

namespace insight {

defineType(kEpsilon_RASModel);
addToFactoryTable(turbulenceModel, kEpsilon_RASModel);
addToOpenFOAMCaseElementFactoryTable(kEpsilon_RASModel);

kEpsilon_RASModel::kEpsilon_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
  : kEpsilonBase_RASModel(c, ip.forward<Parameters>())
{}


} // namespace insight
