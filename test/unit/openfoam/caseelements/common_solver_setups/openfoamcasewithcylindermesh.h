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
#include "openfoam/caseelements/thermodynamics/compressiblesinglephasethermophysicalproperties.h"
#include "openfoam/caseelements/turbulencemodels/komegasst_rasmodel.h"
#include "openfoam/caseelements/thermophysicalcaseelements.h"
#include "openfoam/openfoamtools.h"





class OpenFOAMCaseWithMesh
        : public OpenFOAMTestCase
{
protected:
    arma::mat flowDir_;

public:
  OpenFOAMCaseWithMesh(
        const string& OFEname,
        CaseFeatures exclFeats = {} );

  virtual void createInletBC(OFDictData::dict&);
  virtual void createOutletBC(OFDictData::dict&);
  virtual void createWallBC(OFDictData::dict&);
  virtual void createCaseElements();

  virtual void createMesh();
  virtual void createCase();

  void run() override;

  const boost::filesystem::path& dir() const;
};




template<class Base = OpenFOAMCaseWithMesh>
class OpenFOAMCaseWithCylinderMesh
    : public Base
{
protected:
    bmd::blockMeshDict_Cylinder::Parameters meshParameters_;

public:

    template<typename ... Args>
    OpenFOAMCaseWithCylinderMesh(Args... a)
        : Base(std::forward<Args>(a)...)
    {
        meshParameters_.mesh.basePatchName="inlet";
        meshParameters_.mesh.topPatchName="outlet";
        meshParameters_.mesh.resolution =
            bmd::blockMeshDict_Cylinder::Parameters::mesh_type::resolution_cubical_type{9};
        meshParameters_.mesh.cellZoneName="wholeDomain";
    }

  void createMesh() override
  {
      Base::createMesh();

      OpenFOAMCase meshCase(this->ofe());

      meshCase.insert(new MeshingNumerics(meshCase));

      meshCase.insert(new bmd::blockMeshDict_Cylinder(meshCase, meshParameters_));

      meshCase.createOnDisk(this->dir_);
      meshCase.executeCommand(this->dir_, "blockMesh");
  }
};




template<class Base = OpenFOAMCaseWithMesh>
class OpenFOAMCaseWithBoxMesh
    : public Base
{
protected:
    bmd::blockMeshDict_Box::Parameters meshParameters_;

public:
    template<typename ... Args>
    OpenFOAMCaseWithBoxMesh(Args... a)
        : Base(std::forward<Args>(a)...)
    {
        meshParameters_.geometry.W=0.1;
        meshParameters_.mesh.resolution =
            bmd::blockMeshDict_Box::Parameters::mesh_type::resolution_individual_type
            { 2, 2, 1 };
        meshParameters_.mesh.XmPatchName="inlet";
        meshParameters_.mesh.XpPatchName="outlet";
        meshParameters_.mesh.ZmPatchName="back";
        meshParameters_.mesh.ZpPatchName="front";
        meshParameters_.mesh.defaultPatchName="walls";
    }

    void createMesh() override
    {
        Base::createMesh();

        OpenFOAMCase meshCase(this->ofe());

        meshCase.insert(new MeshingNumerics(meshCase));

        meshCase.insert(new bmd::blockMeshDict_Box(meshCase, meshParameters_));

        meshCase.createOnDisk(this->dir_);
        meshCase.executeCommand(this->dir_, "blockMesh");
    }
};




template<class Base>
class PimpleFoamOpenFOAMCase
    : public Base
{
protected:


public:
    using Base::Base;

    void createCaseElements() override
    {
        Base::createCaseElements();

        if (!this->exclFeats_.count(Numerics))
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
        }

        if (!this->exclFeats_.count(TransportProperties))
        {
            this->insert(new singlePhaseTransportProperties(*this));
        }

        if (!this->exclFeats_.count(TurbulenceModel))
        {
            this->insert(new kOmegaSST_RASModel(*this));
        }
    }
};

typedef
    PimpleFoamOpenFOAMCase<OpenFOAMCaseWithCylinderMesh<> >
        PimpleFoamCylinderOpenFOAMCase;

typedef
    PimpleFoamOpenFOAMCase<OpenFOAMCaseWithBoxMesh<> >
        PimpleFoamBoxOpenFOAMCase;


template<class Base>
class SteadyCompressibleOpenFOAMCase
 : public Base
{

public:
  using Base::Base;

    void createCaseElements() override
    {
        Base::createCaseElements();

        if (!this->exclFeats_.count(Numerics))
        {
            steadyCompressibleNumerics::Parameters p;
            p.pinternal=1e5;
            p.endTime=1;

            this->insert(new steadyCompressibleNumerics(*this, p) );
        }

        if (!this->exclFeats_.count(TransportProperties))
        {
            this->insert(new compressibleSinglePhaseThermophysicalProperties(
                *this,
                compressibleSinglePhaseThermophysicalProperties::Parameters()
                ));
        }

        if (!this->exclFeats_.count(TurbulenceModel))
        {
            this->insert(new kOmegaSST_RASModel(*this));
        }
    }
};




template<class Base>
class SimpleFoamOpenFOAMCase
 : public Base
{
public:
    using Base::Base;

  void createCaseElements() override
  {
      Base::createCaseElements();

      if (!this->exclFeats_.count(Numerics))
      {
          steadyIncompressibleNumerics::Parameters p;
          p.pinternal=1e5;
          p.writeInterval=1;
          p.writeControl=steadyIncompressibleNumerics::Parameters::timeStep;
          p.endTime=1;

          this->insert(new steadyIncompressibleNumerics(*this, p));
      }


      if (!this->exclFeats_.count(TransportProperties))
        this->insert(new singlePhaseTransportProperties(*this));

      if (!this->exclFeats_.count(TurbulenceModel))
          this->insert(new kOmegaSST_RASModel(*this));
  }
};


typedef
    SimpleFoamOpenFOAMCase<OpenFOAMCaseWithCylinderMesh<> >
    SimpleFoamCylinderOpenFOAMCase;

typedef
    SimpleFoamOpenFOAMCase<OpenFOAMCaseWithBoxMesh<> >
        SimpleFoamBoxOpenFOAMCase;


class CyclicPimpleFoamOpenFOAMCase
    : public PimpleFoamOpenFOAMCase<OpenFOAMCaseWithCylinderMesh<> >
{
public:
  using PimpleFoamOpenFOAMCase<OpenFOAMCaseWithCylinderMesh<> >
        ::PimpleFoamOpenFOAMCase;

  void createMesh() override;
  void createInletBC(OFDictData::dict&) override;
  void createOutletBC(OFDictData::dict&) override;
};




#endif // OPENFOAMCASEWITHCYLINDERMESH_H
