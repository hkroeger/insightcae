#include "realizablekepsilon_rasmodel.h"

namespace insight {



defineType(realizablekEpsilon_RASModel);
addToFactoryTable(turbulenceModel, realizablekEpsilon_RASModel);
addToOpenFOAMCaseElementFactoryTable(realizablekEpsilon_RASModel);

realizablekEpsilon_RASModel::realizablekEpsilon_RASModel(OpenFOAMCase& c, const ParameterSet& ps)
  : kEpsilonBase_RASModel(c)
{}


} // namespace insight
