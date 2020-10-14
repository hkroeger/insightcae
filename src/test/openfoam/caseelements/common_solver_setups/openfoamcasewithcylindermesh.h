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

  void runTest() override;
};

#endif // OPENFOAMCASEWITHCYLINDERMESH_H
