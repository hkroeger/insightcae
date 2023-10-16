#include "test_pdl.h"
#include "test_pdl__TestPDL__Parameters_headers.h"


using namespace std;
using namespace insight;

int main()
{
  auto ps_test = TestPDL::Parameters::makeDefault();

  ps_test.get<scalarLengthParameter>("L").valueChanged.connect([]() { cout<<"L changed"<<endl; });
  ps_test.get<ArrayParameter>("ap").valueChanged.connect([]() { cout<<"ap changed"<<endl; });
  ps_test.get<MatrixParameter>("matrix").valueChanged.connect([]() { cout<<"matrix changed"<<endl; });
  ps_test.get<SelectionParameter>("sel").valueChanged.connect([]() { cout<<"sel changed"<<endl; });
  ps_test.get<SpatialTransformationParameter>("trsf").valueChanged.connect([]() { cout<<"trsf changed"<<endl; });
  ps_test.get<DoubleParameter>("run/regime/endTime").valueChanged.connect([]() { cout<<"endTime changed"<<endl; });
  ps_test.get<PathParameter>("mapFrom").valueChanged.connect([]() { cout<<"mapFrom changed"<<endl; });
  ps_test.get<SelectableSubsetParameter>("turbulenceModel").valueChanged.connect([]() { cout<<"turbulenceModel changed"<<endl; });

  ps_test.get<DoubleParameter>("run/regime/endTime").set(1);
  ps_test.get<SelectableSubsetParameter>("turbulenceModel").setSelection("kEpsilon");

  TestPDL::Parameters p_test(ps_test);
  p_test.sel=TestPDL::Parameters::three;
  p_test.set(ps_test);

  cout << p_test.sel << endl;
  cout << p_test.L << endl;

  return 0;
}
