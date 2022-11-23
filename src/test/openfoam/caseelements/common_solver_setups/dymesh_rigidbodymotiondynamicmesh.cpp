#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/dynamicmesh/rigidbodymotiondynamicmesh.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public OpenFOAMCaseWithCylinderMesh
    {
    public:
        Case(const std::string& ofe) : OpenFOAMCaseWithCylinderMesh(ofe)
        {
            meshParameters_.geometry.d = 0.2*meshParameters_.geometry.D;
            meshParameters_.mesh.innerPatchName="inner";
        }

        void createWallBC(OFDictData::dict& bd)
        {
           OpenFOAMCaseWithCylinderMesh::createWallBC(bd);
           insert(new WallBC(*this, "inner", bd));
        }

        void createMesh()
        {
          OpenFOAMCase meshCase(ofe());

          meshCase.insert(new MeshingNumerics(meshCase));

          meshCase.insert(new bmd::blockMeshDict_Cylinder(meshCase, meshParameters_));

          meshCase.createOnDisk(dir_);
          meshCase.executeCommand(dir_, "blockMesh");
        }

        void createCase()
        {
           OpenFOAMCaseWithCylinderMesh::createCase();

           insert(new rigidBodyMotionDynamicMesh(
                      *this,
                      rigidBodyMotionDynamicMesh::Parameters()
                      .set_rigidBodyMotion(rigidBodyMotionDynamicMesh::Parameters::rigidBodyMotion_type()
                      .set_bodies({
                                      {
                                          "body",
                                          meshParameters_.geometry.p0 + 0.5*meshParameters_.geometry.L,
                                          1.0,
                                          100., 100., 100.,
                                          { "inner" },
                                          0.5, 1.,
                                          { rigidBodyMotionDynamicMesh::Parameters::rigidBodyMotion_type::bodies_default_type::Pxyz },
                                          {}
                                      }
                                  })
                          )
                      ));
        }
    };

    PimpleFoamOpenFOAMCase<Case> tc(argv[1]);

    tc.runTest();

  });
}
