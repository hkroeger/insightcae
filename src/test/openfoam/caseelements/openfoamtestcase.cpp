#include "openfoamtestcase.h"
#include "openfoam/ofes.h"


OpenFOAMTestCase::OpenFOAMTestCase(const string &OFEname)
  : OpenFOAMCase(OFEs::get(OFEname))
{}

int executeTest(std::function<void(void)> testFunction)
{
  try
  {
    testFunction();
    return 0;
  }
  catch(const insight::UnsupportedFeature& ue)
  {
    cout<<ue<<endl;
    return 0;
  }
  catch(const std::exception& e)
  {
    cout<<e.what()<<endl;
    return 1;
  }
}
