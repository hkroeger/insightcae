#include "symmetrybc.h"

namespace insight {


defineType(SymmetryBC);
addToFactoryTable(BoundaryCondition, SymmetryBC);
addToStaticFunctionTable(BoundaryCondition, SymmetryBC, defaultParameters);


SymmetryBC::SymmetryBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& )
    : SimpleBC ( c, patchName, boundaryDict, "symmetryPlane" )
{}



} // namespace insight
