#include "dynamicoversetfvmesh.h"


#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"


namespace insight {


defineType(dynamicOversetFvMesh);
addToOpenFOAMCaseElementFactoryTable(dynamicOversetFvMesh);



dynamicOversetFvMesh::dynamicOversetFvMesh( OpenFOAMCase& c, const ParameterSet& ps )
: dynamicMesh(c, ps),
  ps_(ps)
{
}

void dynamicOversetFvMesh::addFields( OpenFOAMCase& c ) const
{
  c.addField
  (
      "pointDisplacement",
       FieldInfo(vectorField, 	dimLength, 	FieldValue({0, 0, 0}), pointField )
  );
}

void dynamicOversetFvMesh::addIntoDictionaries(OFdicts& dictionaries) const
{
    Parameters p(ps_);

    OFDictData::dict& dynamicMeshDict
      = dictionaries.lookupDict("constant/dynamicMeshDict");

    dynamicMeshDict["dynamicFvMesh"]="dynamicOversetFvMesh";

    OFDictData::dict solvers;

    SixDOFRigidBodyMotionSolver rbms(p.rigidBodyMotion);
    std::string name = rbms.motionSolverName();

    OFDictData::dict rbmc;
    rbmc["motionSolver"]=name;
    rbms.addIntoDict(rbmc);
    solvers[name+"Coeffs"]=rbmc;

    dynamicMeshDict["solvers"]=solvers;

    OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
    controlDict.getList("libs").insertNoDuplicate( "\"lib"+name+".so\"" );

}

} // namespace insight
