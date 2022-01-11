#include "openfoamcasewithcylindermesh.h"

#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh_templates.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/boundaryconditions/cyclicpairbc.h"

#include "openfoam/caseelements/numerics/unsteadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/steadycompressiblenumerics.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/turbulencemodels/komegasst_rasmodel.h"
#include "openfoam/caseelements/thermophysicalcaseelements.h"
#include "openfoam/openfoamtools.h"



OpenFOAMCaseWithMesh::OpenFOAMCaseWithMesh(const string &OFEname)
    : OpenFOAMTestCase(OFEname),
      flowDir_(vec3(0,0,1)),
      dir_(false)
{}




void OpenFOAMCaseWithMesh::createInletBC(OFDictData::dict& bd)
{
    VelocityInletBC::Parameters p;
    p.velocity.fielddata=VelocityInletBC::Parameters::velocity_type::fielddata_uniformSteady_type{ 1.0*flowDir_ };
    insert(new VelocityInletBC(*this, "inlet", bd, p));
}




void OpenFOAMCaseWithMesh::createOutletBC(OFDictData::dict& bd)
{
    PressureOutletBC::Parameters p;
    arma::mat p0;
    p0 << 1e5;
    PressureOutletBC::Parameters::behaviour_uniform_type punif;
    punif.pressure.fielddata=PressureOutletBC::Parameters::behaviour_uniform_type::pressure_type::fielddata_uniformSteady_type{ p0 };
    p.behaviour=punif;
    insert(new PressureOutletBC(*this, "outlet", bd, p));
}




void OpenFOAMCaseWithMesh::createWallBC(OFDictData::dict& bd)
{
  insert(new WallBC(*this, "walls", bd));
}




void OpenFOAMCaseWithMesh::createCaseElements()
{}




void OpenFOAMCaseWithMesh::createCase()
{
  OFDictData::dict boundaryDict;
  parseBoundaryDict(dir_, boundaryDict);

  createInletBC(boundaryDict);
  createOutletBC(boundaryDict);
  createWallBC(boundaryDict);
  createCaseElements();
}




void OpenFOAMCaseWithMesh::runTest()
{
  createMesh();

  createCase();

  createOnDisk(dir_);
  executeCommand(dir_, readSolverName(dir_));
}




const boost::filesystem::path& OpenFOAMCaseWithMesh::dir() const
{
  return dir_;
}




OpenFOAMCaseWithCylinderMesh::OpenFOAMCaseWithCylinderMesh(const string &OFEname)
    : OpenFOAMCaseWithMesh(OFEname)
{
    meshParameters_.mesh.basePatchName="inlet";
    meshParameters_.mesh.topPatchName="outlet";
    meshParameters_.mesh.resolution =
            bmd::blockMeshDict_Cylinder::Parameters::mesh_type::resolution_cubical_type{9,0.5};
}

void OpenFOAMCaseWithCylinderMesh::createMesh()
{
  OpenFOAMCase meshCase(ofe());

  meshCase.insert(new MeshingNumerics(meshCase));

  meshCase.insert(new bmd::blockMeshDict_Cylinder(meshCase, meshParameters_));

  meshCase.createOnDisk(dir_);
  meshCase.executeCommand(dir_, "blockMesh");
}





OpenFOAMCaseWithBoxMesh::OpenFOAMCaseWithBoxMesh(const string &OFEname)
    : OpenFOAMCaseWithMesh(OFEname)
{
    meshParameters_.geometry.W=0.1;
    meshParameters_.mesh.resolution =
            bmd::blockMeshDict_Box::Parameters::mesh_type::resolution_individual_type
            { 5, 5, 1 };
    meshParameters_.mesh.XmPatchName="inlet";
    meshParameters_.mesh.XpPatchName="outlet";
    meshParameters_.mesh.ZmPatchName="back";
    meshParameters_.mesh.ZpPatchName="front";
    meshParameters_.mesh.defaultPatchName="walls";
}




void OpenFOAMCaseWithBoxMesh::createMesh()
{
    OpenFOAMCase meshCase(ofe());

    meshCase.insert(new MeshingNumerics(meshCase));

    meshCase.insert(new bmd::blockMeshDict_Box(meshCase, meshParameters_));

    meshCase.createOnDisk(dir_);
    meshCase.executeCommand(dir_, "blockMesh");
}






PimpleFoamOpenFOAMCase::PimpleFoamOpenFOAMCase(const string &OFEname)
  : OpenFOAMCaseWithCylinderMesh(OFEname)
{}



void PimpleFoamOpenFOAMCase::createCaseElements()
{
  unsteadyIncompressibleNumerics::Parameters p;
  p.pinternal=1e5;
  p.deltaT=1e-3;
  p.endTime=1e-3;

  PIMPLESettings::Parameters ti;
  CompressiblePIMPLESettings::Parameters::pressure_velocity_coupling_PIMPLE_type pimple;
  pimple.max_nOuterCorrectors=1;
  pimple.nCorrectors=1;
  ti.pressure_velocity_coupling=pimple;
  p.time_integration=ti;
  insert(new unsteadyIncompressibleNumerics(*this, p));

  insert(new singlePhaseTransportProperties(*this));

  insert(new kOmegaSST_RASModel(*this));
}







SimpleFoamOpenFOAMCase::SimpleFoamOpenFOAMCase(const string &OFEname)
  : OpenFOAMCaseWithCylinderMesh(OFEname)
{}



void SimpleFoamOpenFOAMCase::createCaseElements()
{
  steadyIncompressibleNumerics::Parameters p;
  p.pinternal=1e5;
  p.endTime=1;

  insert(new steadyIncompressibleNumerics(*this, p));

  insert(new singlePhaseTransportProperties(*this));

  insert(new kOmegaSST_RASModel(*this));
}





SteadyCompressibleOpenFOAMCase::SteadyCompressibleOpenFOAMCase(const string &OFEname)
: OpenFOAMCaseWithCylinderMesh(OFEname)
{
}

void SteadyCompressibleOpenFOAMCase::createCaseElements()
{
    steadyCompressibleNumerics::Parameters p;
    p.pinternal=1e5;
    p.endTime=1;

    insert(new steadyCompressibleNumerics(*this, p) );
    insert(new compressibleSinglePhaseThermophysicalProperties(
               *this,
               compressibleSinglePhaseThermophysicalProperties::Parameters()
               ));
    insert(new kOmegaSST_RASModel(*this));
}



CyclicPimpleFoamOpenFOAMCase::CyclicPimpleFoamOpenFOAMCase(const string &OFEname)
  : PimpleFoamOpenFOAMCase(OFEname)
{}

void CyclicPimpleFoamOpenFOAMCase::createMesh()
{
  PimpleFoamOpenFOAMCase::createMesh();

  createPatchOps::createCyclicOperator::Parameters p;
  p.set_name("inout");
  p.set_constructFrom("patches");
  p.set_patches({"inlet"});
  p.set_patches_half1({"outlet"});

  createPatch
  (
    *this, dir_,
    { std::make_shared<createPatchOps::createCyclicOperator>(p) }
  );
}

void CyclicPimpleFoamOpenFOAMCase::createInletBC(OFDictData::dict &boundaryDict)
{
  insert(new CyclicPairBC(*this, "inout", boundaryDict));
}

void CyclicPimpleFoamOpenFOAMCase::createOutletBC(OFDictData::dict &)
{
  // skip
}
