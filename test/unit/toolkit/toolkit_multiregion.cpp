
#include <string>

#include "base/tools.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamanalysis.h"

#include "openfoam/ofes.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/blockmesh.h"

#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/analysiscaseelements.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/symmetrybc.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"
#include "openfoam/caseelements/numerics/chtmultiregionnumerics.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"

#include "openfoam/caseelements/numerics/laplacianfoamnumerics.h"

using namespace std;
using namespace insight;

class MultiregionTestAnalysis
    : public OpenFOAMAnalysis
{
  string
      ninlet="inlet", noutlet="outlet", nfrontback="frontAndBack",
      nrfluid="lower", nrsolid="upper"
      ;

public:
  MultiregionTestAnalysis(const boost::filesystem::path& exepath)
    : OpenFOAMAnalysis(
              std::make_shared<supplementedInputDataBase>(
                  Parameters(), ".", consoleProgressDisplayer ))
  {
  }

  static AnalysisDescription description() { return {"multiregion test", ""}; }

  // create mesh
  void createMesh(OpenFOAMCase& cm, ProgressDisplayer& progress) override
  {
    OpenFOAMCase cmaux;
    cmaux.insert(new MeshingNumerics(cmaux));

    using namespace insight::bmd;
    std::unique_ptr<blockMesh> bmd(new blockMesh(cmaux));
    bmd->setScaleFactor(1);
    bmd->setDefaultPatch("walls", "wall");


    // create patches
    Patch& inlet = 	bmd->addPatch(ninlet, new Patch());
    Patch& outlet = 	bmd->addPatch(noutlet, new Patch());
    Patch& frontback = bmd->addPatch(nfrontback, new Patch());

    arma::mat vh=vec3(0,0,1);

    {
      std::map<int, Point> pts = {

        { 0, 	vec3(0, 0., 0) },
        { 1, 	vec3(1, 0., 0) },
        { 2, 	vec3(0, 1., 0) },
        { 3, 	vec3(1, 1., 0) },
        { 4, 	vec3(0, 2., 0) },
        { 5, 	vec3(1, 2., 0) },
      };

      auto PTS = [&](int a, int b, int c, int d)
      {
           return P_8(
                pts[a], pts[b], pts[c], pts[d],
                pts[a]+vh, pts[b]+vh, pts[c]+vh, pts[d]+vh
              );
      };

      {
        Block& bl = bmd->addBlock
        (
          new Block(PTS(0,1,3,2),
            2, 2, 2, {1,1,1}, nrfluid
          )
        );

        inlet.addFace(bl.face("0473"));
        outlet.addFace(bl.face("1265"));
        frontback.addFace(bl.face("0321"));
        frontback.addFace(bl.face("4567"));
      }
      {
        Block& bl = bmd->addBlock
        (
          new Block(PTS(2,3,5,4),
            2, 2, 2, {1,1,1}, nrsolid
          )
        );


        frontback.addFace(bl.face("0321"));
        frontback.addFace(bl.face("4567"));
      }

    }

    cmaux.insert(bmd.release());
    cmaux.createOnDisk(executionPath());
    cmaux.executeCommand(executionPath(), "blockMesh");
    cmaux.executeCommand(executionPath(), "splitMeshRegions", {"-cellZones", "-overwrite"});
  }


  // set up case
  void createCase(OpenFOAMCase& cm, ProgressDisplayer& progress) override
  {

    chtMultiRegionNumerics::Parameters mrp;
    mrp.writeInterval=1;

    mrp.fluidRegions.push_back(nrfluid);
    mrp.solidRegions.push_back(nrsolid);

    cm.insert(new chtMultiRegionNumerics(cm, mrp));

    // fluid case
    {
      auto cm_fluidPtr = std::make_shared<OpenFOAMCase>(cm.ofe());
      auto& cm_fluid = *cm_fluidPtr;

      OFDictData::dict boundaryDict;
      cm_fluid.parseBoundaryDict(executionPath(), boundaryDict, nrfluid);

      cm_fluid.insert(new steadyIncompressibleNumerics(cm_fluid, steadyIncompressibleNumerics::Parameters()
        .set_writeInterval(100.0)
        .set_purgeWrite(0)
        .set_endTime(1000.0)
        .set_deltaT(1)
      ));
      cm_fluid.insert(new forces(cm_fluid, forces::Parameters()
        .set_rhoInf(1.0)
        .set_patches({"\"(object.*)\""})
      ));
      cm_fluid.insert(new singlePhaseTransportProperties(cm_fluid, singlePhaseTransportProperties::Parameters()
        .set_nu(1e-6)
      ));

      cm_fluid.insert(new SymmetryBC(cm_fluid, nfrontback, boundaryDict));

      cm_fluid.insert(new PressureOutletBC(cm_fluid, noutlet, boundaryDict));

      cm_fluid.insert(new VelocityInletBC(cm_fluid, ninlet, boundaryDict, VelocityInletBC::Parameters()
          .set_velocity( FieldData::uniformSteady(1.0, 0, 0) )
      ));

      cm_fluid.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters() );

  //    insertTurbulenceModel(cm_fluid,
  //      parameters_.get<SelectableSubsetParameter>("fluid/turbulenceModel"));

      cm.addRegionCase(nrfluid, cm_fluidPtr);
    }

    // solid case
    {
      auto cm_solidPtr = std::make_shared<OpenFOAMCase>(cm.ofe());
      auto& cm_solid = *cm_solidPtr;

      OFDictData::dict boundaryDict;
      cm_solid.parseBoundaryDict(executionPath(), boundaryDict, nrsolid);

      cm_solid.insert(new laplacianFoamNumerics(cm_solid));

      cm_solid.insert(new SymmetryBC(cm_solid, nfrontback, boundaryDict));

      cm_solid.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters() );

  //    insertTurbulenceModel(cm_fluid,
  //      parameters_.get<SelectableSubsetParameter>("fluid/turbulenceModel"));

      cm.addRegionCase(nrsolid, cm_solidPtr);
    }

  }

};



int main(int /*argc*/, char*/*argv*/[])
{

  try
  {
    CaseDirectory dir(false);
    MultiregionTestAnalysis ma(dir);
    ma();
  }
  catch (const std::exception& e)
  {
    cerr<<e.what()<<endl;
    return -1;
  }

  return 0;
}
