#ifndef OPENFOAMTESTCASE_H
#define OPENFOAMTESTCASE_H

#include <functional>
#include "openfoam/openfoamcase.h"
#include "base/casedirectory.h"

using namespace std;
using namespace boost;
using namespace insight;





enum CaseFeature
{
    Numerics, TransportProperties, TurbulenceModel,
    InletBoundaryCondition, OutletBoundaryCondition, WallBoundaryCondition,
    DefaultBoundaryConditions
};




typedef std::set<CaseFeature> CaseFeatures;




class OpenFOAMTestCase
 : public OpenFOAMCase
{

protected:
    CaseDirectory dir_;
    CaseFeatures exclFeats_;
public:
  OpenFOAMTestCase(const string& OFEname, CaseFeatures exclFeats = {});

  virtual void run() =0;
  void runTest();
};

int executeTest(std::function<void(void)> testAlgo);

#endif // OPENFOAMTESTCASE_H
