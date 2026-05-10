#include "solidbodymotiondynamicmesh.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {



defineType(solidBodyMotionDynamicMesh);
addToOpenFOAMCaseElementFactoryTable(solidBodyMotionDynamicMesh);



solidBodyMotionDynamicMesh::solidBodyMotionDynamicMesh(
    OpenFOAMCase& c, ParameterSetInput ip )
: dynamicMesh(c, ip.forward<Parameters>()),
    solidBodyMotionFunction(p())
{}


void solidBodyMotionDynamicMesh::addIntoDictionaries(OFdicts& dictionaries) const
{
    OFDictData::dict& dynamicMeshDict
      = dictionaries.lookupDict("constant/dynamicMeshDict");

    dynamicMeshDict["dynamicFvMesh"]="dynamicMotionSolverFvMesh";
    dynamicMeshDict["solver"]="solidBody";
    OFDictData::dict &sbc=dynamicMeshDict.subDict("solidBodyCoeffs");
    sbc["cellZone"]=p().zonename;
    solidBodyMotionFunction::addIntoDictionary(sbc);
}



} // namespace insight
