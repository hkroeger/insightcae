#ifndef OPENFOAMTESTCASE_H
#define OPENFOAMTESTCASE_H

#include "openfoam/openfoamcase.h"

using namespace std;
using namespace boost;
using namespace insight;

class OpenFOAMTestCase
 : public OpenFOAMCase
{
public:
  OpenFOAMTestCase(const string& OFEname);

  virtual void runTest() =0;
};

#endif // OPENFOAMTESTCASE_H
