#include "turbulencemodel.h"

namespace insight {




defineType(turbulenceModel);
defineFactoryTable(
    turbulenceModel,
    LIST(OpenFOAMCase& ofc, ParameterSetInput&& ip),
    LIST(ofc, std::move(ip))
    );




turbulenceModel::turbulenceModel(OpenFOAMCase& c, ParameterSetInput ip)
: OpenFOAMCaseElement(c, ip.forward<Parameters>())
{}




} // namespace insight
