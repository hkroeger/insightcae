#include "kepsilon_rasmodel.h"

namespace insight {

defineType(kEpsilon_RASModel);
addToFactoryTable(turbulenceModel, kEpsilon_RASModel);
addToOpenFOAMCaseElementFactoryTable(kEpsilon_RASModel);

kEpsilon_RASModel::kEpsilon_RASModel(OpenFOAMCase& c, const ParameterSet& ps)
  : kEpsilonBase_RASModel(c)
{}


} // namespace insight
