#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/boundaryconditions/ggibc.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");


    class Case : public OpenFOAMCaseWithMesh
    {
    public:
        Case(const std::string& ofe) : OpenFOAMCaseWithMesh(ofe) {}

        void createMesh() override
        {
            bmd::blockMeshDict_Cylinder::Parameters mp1;
            mp1.mesh.basePatchName="inlet";
            mp1.mesh.topPatchName="ggi1";
            mp1.mesh.resolution =
                    bmd::blockMeshDict_Cylinder::Parameters::mesh_type::resolution_cubical_type{5,0.5};

            bmd::blockMeshDict_Cylinder::Parameters mp2;
            mp2.geometry.p0 =
                    mp1.geometry.p0 + mp1.geometry.ex*mp1.geometry.L;
            mp2.mesh.basePatchName="ggi2";
            mp2.mesh.topPatchName="outlet";
            mp2.mesh.resolution =
                    bmd::blockMeshDict_Cylinder::Parameters::mesh_type::resolution_cubical_type{5,0.5};


            OpenFOAMCase mc1(ofe());
            mc1.insert(new MeshingNumerics(mc1));
            mc1.insert(new bmd::blockMeshDict_Cylinder(mc1, mp1));

            mc1.createOnDisk(dir_);
            mc1.executeCommand(dir_, "blockMesh");

            OpenFOAMCase mc2(ofe());
            mc2.insert(new MeshingNumerics(mc2));
            mc2.insert(new bmd::blockMeshDict_Cylinder(mc2, mp2));

            auto mc2dir = CaseDirectory::makeTemporary();
            mc2.createOnDisk(*mc2dir);
            mc2.executeCommand(*mc2dir, "blockMesh");

            mergeMeshes(mc1, *mc2dir, dir_);
            resetMeshToLatestTimestep(mc1, dir_);
        }

        void createWallBC(OFDictData::dict& bd) override
        {
            OpenFOAMCaseWithMesh::createWallBC(bd);

            insert(new GGIBC(*this, "ggi1", bd, GGIBC::Parameters()
                             .set_shadowPatch("ggi2")
                             .set_zone("ggi1")
                             ));
            insert(new GGIBC(*this, "ggi2", bd, GGIBC::Parameters()
                             .set_shadowPatch("ggi1")
                             .set_zone("ggi2")
                             ));
        }

    };

    SimpleFoamOpenFOAMCase<Case> tc(argv[1]);

    tc.runTest();
  });
}
