#ifndef OPENFOAMCASEWITHCYLINDERMESH_H
#define OPENFOAMCASEWITHCYLINDERMESH_H

#include "../openfoamtestcase.h"

#include "base/tools.h"
#include "base/casedirectory.h"
#include "openfoam/blockmesh_templates.h"


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


class OpenFOAMCaseWithMesh
        : public OpenFOAMTestCase
{
protected:
    arma::mat flowDir_;
    CaseDirectory dir_;

public:
  OpenFOAMCaseWithMesh(const string& OFEname);

  virtual void createInletBC(OFDictData::dict&);
  virtual void createOutletBC(OFDictData::dict&);
  virtual void createWallBC(OFDictData::dict&);
  virtual void createCaseElements();

  virtual void createMesh() =0;
  virtual void createCase();

  void runTest() override;

  const boost::filesystem::path& dir() const;
};





class OpenFOAMCaseWithCylinderMesh
        : public OpenFOAMCaseWithMesh
{
protected:
    bmd::blockMeshDict_Cylinder::Parameters meshParameters_;

public:
  OpenFOAMCaseWithCylinderMesh(const string& OFEname);

  virtual void createMesh();
};





class OpenFOAMCaseWithBoxMesh
        : public OpenFOAMCaseWithMesh
{
protected:
    bmd::blockMeshDict_Box::Parameters meshParameters_;

public:
  OpenFOAMCaseWithBoxMesh(const string& OFEname);

  virtual void createMesh();
};




template<class Base>
class PimpleFoamOpenFOAMCase
 : public Base
{

public:
  PimpleFoamOpenFOAMCase(const string& OFEname)
      : Base(OFEname)
    {}

  void createCaseElements() override
  {
    unsteadyIncompressibleNumerics::Parameters p;
    p.pinternal=1e5;
    p.writeControl=unsteadyIncompressibleNumerics::Parameters::timeStep;
    p.writeInterval=1;
    p.deltaT=1e-3;
    p.endTime=1e-3;

    PIMPLESettings::Parameters ti;
    CompressiblePIMPLESettings::Parameters::pressure_velocity_coupling_PIMPLE_type pimple;
    ti.timestep_control=PIMPLESettings::Parameters::timestep_control_fixed_type{};
    pimple.max_nOuterCorrectors=1;
    pimple.nCorrectors=1;
    ti.pressure_velocity_coupling=pimple;
    p.time_integration=ti;
    this->insert(new unsteadyIncompressibleNumerics(*this, p));

    this->insert(new singlePhaseTransportProperties(*this));

    this->insert(new kOmegaSST_RASModel(*this));
  }
};

typedef PimpleFoamOpenFOAMCase<OpenFOAMCaseWithCylinderMesh> PimpleFoamCylinderOpenFOAMCase;


class SteadyCompressibleOpenFOAMCase
 : public OpenFOAMCaseWithCylinderMesh
{

public:
  SteadyCompressibleOpenFOAMCase(const string& OFEname);

  void createCaseElements() override;
};



template<class Base>
class SimpleFoamOpenFOAMCase
 : public Base
{

public:
  SimpleFoamOpenFOAMCase(const string& OFEname)
      : Base(OFEname)
  {}


  void createCaseElements() override
  {
      steadyIncompressibleNumerics::Parameters p;
      p.pinternal=1e5;
      p.writeInterval=1;
      p.writeControl=steadyIncompressibleNumerics::Parameters::timeStep;
      p.endTime=1;

      this->insert(new steadyIncompressibleNumerics(*this, p));

      this->insert(new singlePhaseTransportProperties(*this));

      this->insert(new kOmegaSST_RASModel(*this));
  }
};

typedef SimpleFoamOpenFOAMCase<OpenFOAMCaseWithCylinderMesh> SimpleFoamCylinderOpenFOAMCase;


class CyclicPimpleFoamOpenFOAMCase
    : public PimpleFoamOpenFOAMCase<OpenFOAMCaseWithCylinderMesh>
{
public:
  CyclicPimpleFoamOpenFOAMCase(const string& OFEname);

  void createMesh() override;
  void createInletBC(OFDictData::dict&) override;
  void createOutletBC(OFDictData::dict&) override;
};

#endif // OPENFOAMCASEWITHCYLINDERMESH_H
