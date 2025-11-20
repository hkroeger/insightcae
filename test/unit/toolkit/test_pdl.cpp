#include "test_pdl.h"
#include "base/exception.h"
#include "base/parameters/pathparameter.h"
#include "base/units.h"
#include "boost/signals2/connection.hpp"
#include "base/casedirectory.h"
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
    return ip.fileName()==sp->fileName();
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
    ip.resize(tv.size(), false);
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
    ip.assignFrom(*tv);
}

template<class TestValue>
void assign(insight::SelectableSubsetParameter& ip, const TestValue& tv)
{
    ip.setSelectionFromIndex(tv);
}

struct Checker
{

    std::unique_ptr<ParameterSet> psPtr, psModPtr;
    ParameterSet& ps;
    TestPDL::Parameters ps_static;

    Checker(std::unique_ptr<ParameterSet>&& psp)
      : psPtr(std::move(psp)),
        ps(*psPtr),
        ps_static(ps)
    {
        std::cout << "=== GOT ======\n"
                  << ps<<std::endl;

        psModPtr = ps_static.cloneParameterSet();
    }

    template<class InsightParameter, class StaticParameter, class TestValue>
    void check(const std::string& path, StaticParameter& sp, const TestValue& testValue)
    {
        bool gotSignal=false;
        auto &ip = ps.get<InsightParameter>(path);
        auto conn = boost::signals2::scoped_connection(
            ip.valueChanged.connect(
                [this,path,&gotSignal]()
                {
                    DBG_SLOT(valueChanged);

                    gotSignal=true; }
                ));

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

         // fill dyn set from static repr.
        auto ps_modP = ps_static.cloneParameterSet();
        auto &ip_mod = ps_modP->get<InsightParameter>(path);
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

        // collect all test changes into separate PS
        auto &ip_collmod = psModPtr->get<InsightParameter>(path);
        assign(ip_collmod, testValue);
    }

};




int main()
{
    try
    {

        Checker c(TestPDL::Parameters::makeDefault());

        auto& ps_test=c.ps;
        auto& p_test=c.ps_static;

        // check correspondence between dyn/static representation
        {
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
        }

        // ps_test.get<MatrixParameter>("matrix").valueChanged.connect([]() { cout<<"matrix changed"<<endl; });
        // ps_test.get<SelectableSubsetParameter>("turbulenceModel").valueChanged.connect([]() { cout<<"turbulenceModel changed"<<endl; });

        // ps_test.get<DoubleParameter>("run/regime/endTime").set(1);


        // check relative path access
        {
            ps_test.setDouble("run/initialization/preRuns/resolutions/1/nax_parameter", 0.2);
            auto &ae = ps_test.get<DoubleParameter>("run/initialization/preRuns/resolutions/1/nax_parameter");
            cout<<"naxp1="<<ae()<<std::endl;

            c.check<DoubleParameter>(
                  "run/regime/endTime",
                   boost::get<TestPDL::Parameters::run_type::regime_unsteady_type>(
                       c.ps_static.run.regime).endTime,
                   20.);

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

            std::cout<<"subps/W => "<<p_test.subps.W.parameterPath<<std::endl;
            std::cout<<"subps/subsub/sarr => "<<p_test.subps.subsub.sarr.parameterPath<<std::endl;


            ParametersReference<SubPS::Parameters> psub(p_test.subps);

            insight::assertion(
                *psub.get().W.parameterPath=="subps/W",
                "unexpected path");
            insight::assertion(
                *psub.get().subsub.sarr.parameterPath=="subps/subsub/sarr",
                "unexpected path");


            const ParametersBase& ps = p_test.subps;

            std::cout<<*dynamic_cast<const SubPS::Parameters&>(ps).W.parameterPath<<std::endl;
            insight::assertion(
                *dynamic_cast<const SubPS::Parameters&>(ps).W.parameterPath=="subps/W",
                "unexpected path");
        }


        //
        //
        //   save and restore
        //
        //

        {
            CaseDirectory tmp(false);

            c.psModPtr->saveToFile(tmp/"test.ist");

            auto orgps = TestPDL::Parameters::makeDefault();
            orgps->readFromFile(tmp/"test.ist");

            if (!c.psModPtr->isEqual(*orgps))
            {
                std::cout<<"saved:\n>>>"<<*c.psModPtr<<"\n<<<"<<std::endl;
                std::cout<<"retrieved:\n>>>"<<*orgps<<"\n<<<"<<std::endl;
                throw insight::Exception("ParameterSet not properly restored from file");
            }
        }
    }
    catch (std::exception& ex)
    {
        std::cerr<<"failed: "<<ex.what()<<std::endl;
        return -1;
    }



    std::cout<<"Test successful"<<std::endl;
    return 0;
}
