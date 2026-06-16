#include "kepsilon_rasmodel.h"

namespace insight {

defineType(kEpsilon_RASModel);
addToFactoryTable(turbulenceModel, kEpsilon_RASModel);
addToOpenFOAMCaseElementFactoryTable(kEpsilon_RASModel);

kEpsilon_RASModel::kEpsilon_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
  : kEpsilonBase_RASModel(c, ip.forward<Parameters>())
{}


defineType(continuousGasKEpsilon_RASModel);
addToFactoryTable(turbulenceModel, continuousGasKEpsilon_RASModel);
addToOpenFOAMCaseElementFactoryTable(continuousGasKEpsilon_RASModel);

continuousGasKEpsilon_RASModel::continuousGasKEpsilon_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
    : kEpsilonBase_RASModel(c, ip.forward<Parameters>())
{}


defineType(LaheyKEpsilon_RASModel);
addToFactoryTable(turbulenceModel, LaheyKEpsilon_RASModel);
addToOpenFOAMCaseElementFactoryTable(LaheyKEpsilon_RASModel);

LaheyKEpsilon_RASModel::LaheyKEpsilon_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
    : kEpsilonBase_RASModel(c, ip.forward<Parameters>())
{}


} // namespace insight
