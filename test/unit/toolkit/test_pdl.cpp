#include "test_pdl.h"
#include "base/exception.h"
#include "base/parameters/pathparameter.h"
#include "base/units.h"
#include "boost/signals2/connection.hpp"
#include "test_pdl__TestPDL__Parameters_headers.h"
#include <memory>
#include <string>

namespace insight
{
}


using namespace std;
using namespace insight;




template<class Function>
void overAll(const Parameter& p, Function f, int level=0)
{
    f(p, level);
    for (auto &cp: p)
    {
        overAll(cp, f, level+1);
    }
}




template<class InsightParameter, class StaticParameter>
bool testEquality(const InsightParameter& ip, const StaticParameter& sp)
{
    return ip()==sp;
}

template<class StaticParameter>
bool testEquality(const insight::ArrayParameter& ip, const StaticParameter& sp)
{
    bool equal=ip.size()==sp.size();
    if (!equal) return false;
    for (size_t i=0; i< ip.size(); ++i)
    {
        equal=equal&&(dynamic_cast<const DoubleParameter&>(ip[i])()==double(sp[i]));
    }
    return equal;
}

template<class StaticParameter>
bool testEquality(const insight::DoubleRangeParameter& ip, const StaticParameter& sp)
{
    bool equal=ip.values().size()==sp.size();
    if (!equal) return false;
    for (auto val: sp)
    {
        equal=equal&&(ip.values().count(val)>0);
    }
    return equal;
}

template<class StaticParameter>
bool testEquality(const MatrixParameter& ip, const StaticParameter& sp)
{
    return norm(ip()-sp, 2)<1e-12;
}

template<class StaticParameter>
bool testEquality(const PathParameter& ip, const StaticParameter& sp)
{
    return ip.originalFilePath()==sp->originalFilePath();
}

template<class StaticParameter>
bool testEquality(const SpatialTransformationParameter& ip, const StaticParameter& sp)
{
    return !(ip()!=sp);
}

template<class StaticParameter>
bool testEquality(const SelectableSubsetParameter& ip, const StaticParameter& sp)
{
    return ip.selectionIndex()==sp.which();
}

template<>
bool testEquality(const SelectableSubsetParameter& ip, const int& sp)
{
    return ip.selectionIndex()==sp;
}


template<class InsightParameter, class TestValue>
void assign(InsightParameter& ip, const TestValue& tv)
{
    ip.set(tv);
}

template<class TestValue>
void assign(insight::ArrayParameter& ip, const TestValue& tv)
{
    ip.resize(tv.size());
    for (size_t i=0; i< tv.size(); ++i)
        dynamic_cast<DoubleParameter&>(ip[i]).set(double(tv[i]));
}

template<class TestValue>
void assign(insight::DoubleRangeParameter& ip, const TestValue& tv)
{
    ip.resetValues(tv);
}

template<class TestValue>
void assign(insight::MatrixParameter& ip, const TestValue& tv)
{
    ip.set(tv);
}

template<class TestValue>
void assign(insight::PathParameter& ip, const TestValue& tv)
{
    ip=*tv;
}

template<class TestValue>
void assign(insight::SelectableSubsetParameter& ip, const TestValue& tv)
{
    ip.setSelectionFromIndex(tv);
}

struct Checker
{

    std::unique_ptr<ParameterSet> psPtr;
    ParameterSet& ps;
    TestPDL::Parameters ps_static;

    Checker(std::unique_ptr<ParameterSet>&& psp)
      : psPtr(std::move(psp)),
        ps(*psPtr),
        ps_static(ps)
    {
        std::cout << "=== GOT ======\n"
                  << ps<<std::endl;
    }

    template<class InsightParameter, class StaticParameter, class TestValue>
    void check(const std::string& path, StaticParameter& sp, const TestValue& testValue)
    {
        bool gotSignal=false;
        auto &ip = ps.get<InsightParameter>(path);
        auto conn = boost::signals2::scoped_connection(
            ip.valueChanged.connect(
                [this,path,&gotSignal]()
                { cout<<path<<" changed"<<endl; gotSignal=true; }));

        // test if properly initialized
        insight::assertion(
            testEquality(ip, sp),
            "parameters are not equal in static and dynamic representation for %s", path.c_str());
        insight::assertion(
            bool(sp.parameterPath),
            "static representation does not contain a parameter path. Expected \"%s\"",
            path.c_str());
        insight::assertion(
            path==sp.parameterPath.value(),
            "static representation does not contain proper path. Expected \"%s\", got \"%s\"",
            path.c_str(), sp.parameterPath.value().c_str());

        sp=testValue;

        auto ps_modP = ps_static.cloneParameterSet(); // fill dyn set from static repr.
        auto &ps_mod = *ps_modP;
        auto &ip_mod = ps_mod.get<InsightParameter>(path);
        insight::assertion(
            testEquality(ip_mod, testValue),
            "modified value not properly transferred for %s", path.c_str());

        assign(ip, testValue);

        insight::assertion(
            gotSignal,
            "did not receive value change acknowledgement for %s", path.c_str());

        ps_static.get(ps); // fill static set from dyn repr.
        insight::assertion(
            testEquality(ip, sp),
            "parameter in static set is properly copied from dynamic set %s", path.c_str());
    }

};




int main()
{
  Checker c(TestPDL::Parameters::makeDefault());

  auto& ps_test=c.ps;
  auto& p_test=c.ps_static;


  c.check<scalarLengthParameter>("L", c.ps_static.L, si::Length(555.*si::meter) );

  c.check<DoubleParameter>("run/regime/endTime",
                           boost::get<TestPDL::Parameters::run_type::regime_unsteady_type>(
                               c.ps_static.run.regime).endTime,
                           20.);

  c.check<DoubleParameter>("ap/1", c.ps_static.ap[1], 555. );
  c.check<ArrayParameter>("ap", c.ps_static.ap, std::vector<PrimitiveStaticValueWrap<double> >{{5.},{5.},{5.}} );

  c.check<DoubleRangeParameter>("dr", c.ps_static.dr,  std::set<double>{4,5,6,7});

  c.check<MatrixParameter>("matrix", c.ps_static.matrix,  arma::mat{{1,2,3},{4,5,6},{7,8,9}});
  c.check<SelectionParameter>("sel", c.ps_static.sel, TestPDL::Parameters::sel_type::three );
  c.check<PathParameter>("mapFrom", c.ps_static.mapFrom, insight::make_filepath("xx.iscad") );
  c.check<SpatialTransformationParameter>("trsf", c.ps_static.trsf, SpatialTransformation(555.) );

  // c.check<SelectableSubsetParameter>("turbulenceModel", c.ps_static.turbulenceModel, 2 );
  // c.check<SelectableSubsetParameter>("run/regime", c.ps_static.run.regime,
  //       TestPDL::Parameters::run_type::regime_steady_type{} );

  //c.check<>("", c.ps_static.,  );

  // ps_test.get<MatrixParameter>("matrix").valueChanged.connect([]() { cout<<"matrix changed"<<endl; });
  // ps_test.get<SelectableSubsetParameter>("turbulenceModel").valueChanged.connect([]() { cout<<"turbulenceModel changed"<<endl; });

  // ps_test.get<DoubleParameter>("run/regime/endTime").set(1);


  std::cout << "major check finished" << std::endl;

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
