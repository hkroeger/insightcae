#ifndef OPENFOAMCASEWITHCYLINDERMESH_H
#define OPENFOAMCASEWITHCYLINDERMESH_H

#include "../openfoamtestcase.h"

#include "base/tools.h"
#include "base/casedirectory.h"
#include "openfoam/blockmesh_templates.h"



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




class PimpleFoamOpenFOAMCase
 : public OpenFOAMCaseWithCylinderMesh
{

public:
  PimpleFoamOpenFOAMCase(const string& OFEname);

  void createCaseElements() override;
};

class SteadyCompressibleOpenFOAMCase
 : public OpenFOAMCaseWithCylinderMesh
{

public:
  SteadyCompressibleOpenFOAMCase(const string& OFEname);

  void createCaseElements() override;
};

class SimpleFoamOpenFOAMCase
 : public OpenFOAMCaseWithCylinderMesh
{

public:
  SimpleFoamOpenFOAMCase(const string& OFEname);

  void createCaseElements() override;
};


class CyclicPimpleFoamOpenFOAMCase
    : public PimpleFoamOpenFOAMCase
{
public:
  CyclicPimpleFoamOpenFOAMCase(const string& OFEname);

  void createMesh() override;
  void createInletBC(OFDictData::dict&) override;
  void createOutletBC(OFDictData::dict&) override;
};

#endif // OPENFOAMCASEWITHCYLINDERMESH_H
