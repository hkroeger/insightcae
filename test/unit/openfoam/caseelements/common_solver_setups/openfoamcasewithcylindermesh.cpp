#include "openfoamcasewithcylindermesh.h"
#include "openfoam/createpatch.h"
#include "openfoam/caseelements/thermodynamics/compressiblesinglephasethermophysicalproperties.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"


OpenFOAMCaseWithMesh::OpenFOAMCaseWithMesh(
    const string &OFEname, CaseFeatures exclFeats)
    : OpenFOAMTestCase(OFEname, exclFeats),
      flowDir_(vec3(0,0,1))
{}




void OpenFOAMCaseWithMesh::createInletBC(OFDictData::dict& bd)
{
    if (!exclFeats_.count(InletBoundaryCondition))
    {
        VelocityInletBC::Parameters p;
        p.velocity.fielddata=VelocityInletBC::Parameters
            ::velocity_type
            ::fielddata_uniformSteady_type{ 1.0*flowDir_ };
        insert(new VelocityInletBC(*this, "inlet", bd, p));
    }
}




void OpenFOAMCaseWithMesh::createOutletBC(OFDictData::dict& bd)
{
    if (!exclFeats_.count(OutletBoundaryCondition))
    {
        PressureOutletBC::Parameters p;
        arma::mat p0;
        p0 << 1e5;
        PressureOutletBC::Parameters::behaviour_uniform_type punif;
        punif.pressure.fielddata=PressureOutletBC
            ::Parameters
            ::behaviour_uniform_type
            ::pressure_type
            ::fielddata_uniformSteady_type{ p0 };
        p.behaviour=punif;
        insert(new PressureOutletBC(*this, "outlet", bd, p));
    }
}




void OpenFOAMCaseWithMesh::createWallBC(OFDictData::dict& bd)
{
    if (!exclFeats_.count(WallBoundaryCondition))
    {
        insert(new WallBC(*this, "walls", bd));
    }
}




void OpenFOAMCaseWithMesh::createCaseElements()
{}



void OpenFOAMCaseWithMesh::createMesh()
{}



void OpenFOAMCaseWithMesh::createCase()
{
  OFDictData::dict boundaryDict;
  parseBoundaryDict(dir_, boundaryDict);

  createInletBC(boundaryDict);
  createOutletBC(boundaryDict);
  createWallBC(boundaryDict);
  createCaseElements();
  if (!exclFeats_.count(DefaultBoundaryConditions))
  {
      addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters() );
  }
}




void OpenFOAMCaseWithMesh::run()
{
  createMesh();

  createCase();

  modifyMeshOnDisk(dir_);
  createOnDisk(dir_);
  modifyCaseOnDisk(dir_);
  executeCommand(dir_, readSolverName(dir_));
}




const boost::filesystem::path& OpenFOAMCaseWithMesh::dir() const
{
  return dir_;
}








void CyclicPimpleFoamOpenFOAMCase::createMesh()
{
  PimpleFoamOpenFOAMCase<OpenFOAMCaseWithCylinderMesh<> >::createMesh();

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
