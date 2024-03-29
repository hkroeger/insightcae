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

  cout << ps_test.getDouble("run/regime/endTime") << endl;

  ps_test.setDouble("run/initialization/preRuns/resolutions/1/nax_parameter", 0.2);
  auto &ae = ps_test.get<DoubleParameter>("run/initialization/preRuns/resolutions/1/nax_parameter");
  cout<<"naxp1="<<ae()<<std::endl;

  bool failed=false;
  try
  {
      ae.parentSet().get<DoubleParameter>("../../../0/nax_parameter");
  }
  catch (...)
  {
      failed=true;
  }
  if (!failed)
      throw insight::Exception("acces should have failed!");

  auto &ae2 = ae.parentSet().get<DoubleParameter>("../0/nax_parameter");
  cout<<"naxp0="<<ae2()<<std::endl;

  auto &mp = ae.parentSet().get<MatrixParameter>("../../../../../matrix");
  cout<<"matrix="<<mp()<<std::endl;

  return 0;
}
