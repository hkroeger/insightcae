#ifndef OPENFOAMCASEWITHCYLINDERMESH_H
#define OPENFOAMCASEWITHCYLINDERMESH_H

#include "../openfoamtestcase.h"

#include "base/tools.h"

class OpenFOAMCaseWithCylinderMesh
        : public OpenFOAMTestCase
{
protected:
    arma::mat flowDir_;
    CaseDirectory dir_;

public:
  OpenFOAMCaseWithCylinderMesh(const string& OFEname);

  virtual void createInletBC(OFDictData::dict&);
  virtual void createOutletBC(OFDictData::dict&);
  virtual void createWallBC(OFDictData::dict&);
  virtual void createCaseElements();

  virtual void createMesh();
  virtual void createCase();
  void runTest() override;

  const boost::filesystem::path& dir() const;
};





class PimpleFoamOpenFOAMCase
 : public OpenFOAMCaseWithCylinderMesh
{

public:
  PimpleFoamOpenFOAMCase(const string& OFEname);

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
