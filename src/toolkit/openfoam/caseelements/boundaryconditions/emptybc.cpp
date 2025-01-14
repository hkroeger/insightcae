#include "emptybc.h"

namespace insight {


defineType(EmptyBC);
addToFactoryTable(BoundaryCondition, EmptyBC);
addToStaticFunctionTable(BoundaryCondition, EmptyBC, defaultParameters);


EmptyBC::EmptyBC (
    OpenFOAMCase& c, const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip )
    : SimpleBC ( c, patchName, boundaryDict, "empty" )
{}


} // namespace insight
