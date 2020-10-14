#include "openfoamtestcase.h"
#include "openfoam/ofes.h"


OpenFOAMTestCase::OpenFOAMTestCase(const string &OFEname)
  : OpenFOAMCase(OFEs::get(OFEname))
{}
