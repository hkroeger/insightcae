#ifndef OPENFOAMTESTCASE_H
#define OPENFOAMTESTCASE_H

#include <functional>
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

int executeTest(std::function<void(void)> testAlgo);

#endif // OPENFOAMTESTCASE_H
