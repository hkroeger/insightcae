#include "rigidbodymotiondynamicmesh.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {

defineType(rigidBodyMotionDynamicMesh);
addToOpenFOAMCaseElementFactoryTable(rigidBodyMotionDynamicMesh);



rigidBodyMotionDynamicMesh::rigidBodyMotionDynamicMesh( OpenFOAMCase& c, const ParameterSet& ps )
: dynamicMesh(c, ps),
  ps_(ps)
{
}

void rigidBodyMotionDynamicMesh::addFields( OpenFOAMCase& c ) const
{
  c.addField
  (
      "pointDisplacement",
       FieldInfo(vectorField, 	dimLength, 	FieldValue({0, 0, 0}), pointField )
  );
}

void rigidBodyMotionDynamicMesh::addIntoDictionaries(OFdicts& dictionaries) const
{
    Parameters p(ps_);

    OFDictData::dict& dynamicMeshDict
      = dictionaries.lookupDict("constant/dynamicMeshDict");

    dynamicMeshDict["dynamicFvMesh"]="dynamicMotionSolverFvMesh";


    SixDOFRigidBodyMotionSolver rbms(p.rigidBodyMotion);
    std::string name = rbms.motionSolverName();

    if (OFversion()<=650)
    {
        OFDictData::dict rbmc;
        rbms.addIntoDict(rbmc);
        dynamicMeshDict[name+"Coeffs"]=rbmc;
        dynamicMeshDict["solver"]=name;
    }
    else
    {
        rbms.addIntoDict(dynamicMeshDict);
        dynamicMeshDict["motionSolver"]=name;
    }


    OFDictData::list libl;
    libl.push_back("\"lib"+name+".so\"");
    libl.push_back("\"libextendedRigidBodyDynamics.so\"");
    dynamicMeshDict["motionSolverLibs"]=libl;


    if (p.moveMeshOuterCorrectors)
    {
        OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
        OFDictData::dict& PIMPLE=fvSolution.subDict("PIMPLE");
        PIMPLE["moveMeshOuterCorrectors"]=true;
    }
}


} // namespace insight
