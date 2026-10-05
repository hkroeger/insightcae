#include "base/supplementedinputdata.h"
#include "base/exception.h"

#include <iostream>
#include <atomic>

using namespace std;
using namespace insight;


/**
 * diamond dependency:
 *   a; b(a); c(a); d(b,c); e independent
 */
struct DiamondSID
    : public supplementedInputDataBase
{
    Supplemented<double> a_, b_, c_, d_, e_;

    std::atomic<int> concurrent{0}, maxConcurrent{0};

    void enterSection()
    {
        int n=++concurrent;
        int m=maxConcurrent;
        while (n>m && !maxConcurrent.compare_exchange_weak(m, n)) {}
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
        --concurrent;
    }

    DiamondSID(bool failB)
        : supplementedInputDataBase(boost::filesystem::path("."))
    {
        a_ = deferred("a", [this]() { return 1.; });
        b_ = deferred("b", [this,failB]()
                      {
                          enterSection();
                          if (failB) throw insight::Exception("b is broken");
                          return a_+1.;
                      });
        c_ = deferred("c", [this](ActionProgress& ap)
                      {
                          ap.message("computing c");
                          enterSection();
                          return a_+2.;
                      });
        d_ = deferred("d", [this]() { return b_+c_; });
        e_ = deferred("e", [this]() { return 10.; });

        reportSupplementQuantity("b", b_, "value of b");
        reportSupplementQuantity("c", c_, "value of c");
        reportSupplementQuantity("plain", 3., "value of plain");
    }
};


/**
 * chain of dependencies, longer than the number of worker threads
 * (checks for absence of deadlocks)
 */
struct ChainSID
    : public supplementedInputDataBase
{
    std::vector<Supplemented<int> > q_;

    ChainSID(int n)
        : supplementedInputDataBase(boost::filesystem::path("."))
    {
        q_.resize(n);
        // register in reverse order: workers start with the most dependent quantity
        for (int i=n-1; i>=0; --i)
        {
            q_[i] = deferred(
                "q"+std::to_string(i),
                [this,i]() -> int { return i==0 ? 0 : q_[i-1].get()+1; } );
        }
    }
};


struct CyclicSID
    : public supplementedInputDataBase
{
    Supplemented<int> x_;

    CyclicSID()
        : supplementedInputDataBase(boost::filesystem::path("."))
    {
        x_ = deferred("x", [this]() -> int { return x_.get()+1; } );
    }
};


template<class F>
std::string expectThrow(F f)
{
    try
    {
        f();
    }
    catch (const std::exception& e)
    {
        return e.what();
    }
    throw insight::Exception("expected an exception");
}


int main()
{
    try
    {
        {
            cout<<"== successful, parallel"<<endl;
            DiamondSID sid(false);
            sid.computeAll();
            insight::assertion(sid.complete(), "expected complete");
            insight::assertion(sid.errors().empty(), "expected no errors");
            insight::assertion(fabs(sid.d_-5.)<1e-10, "unexpected value of d: %g", double(sid.d_));
            if (boost::thread::hardware_concurrency()>1)
            {
                insight::assertion(
                    sid.maxConcurrent==2,
                    "expected b and c to be computed in parallel (max. concurrency %d)",
                    int(sid.maxConcurrent) );
            }
            sid.throwIfIncomplete();

            auto t=sid.reportedSupplementQuantities();
            insight::assertion(t.size()==3, "expected 3 reported quantities");
            insight::assertion(boost::get<double>(t.at("c").value)==3., "unexpected reported value");
        }

        {
            cout<<"== failure in b"<<endl;
            DiamondSID sid(true);
            sid.computeAll();
            insight::assertion(!sid.complete(), "expected incomplete");
            insight::assertion(sid.errors().size()==2, "expected b and d to fail, got %d errors", int(sid.errors().size()));
            insight::assertion(sid.rootErrors().size()==1, "expected one root error");
            insight::assertion(sid.rootErrors()[0].first=="b", "expected root error in b");

            // independent quantities are available
            insight::assertion(sid.c_.ok() && sid.c_==3., "c should be available");
            insight::assertion(sid.e_.ok(), "e should be available");
            insight::assertion(!sid.d_.ok(), "d should have failed");

            auto msg=expectThrow([&]() { double v=sid.d_; (void)v; });
            cout<<"access to d: "<<msg<<endl;
            insight::assertion(
                msg.find("\"b\"")!=std::string::npos && msg.find("b is broken")!=std::string::npos,
                "error message of d should name the root cause" );

            try
            {
                double v=sid.d_; (void)v;
            }
            catch (const SupplementedQuantityError& e)
            {
                insight::assertion(e.isDependencyFailure(), "expected dependency failure");
                insight::assertion(e.rootQuantity()=="b", "expected root quantity b");
            }

            msg=expectThrow([&]() { sid.throwIfIncomplete(); });
            cout<<"aggregated: "<<msg<<endl;
            insight::assertion(msg.find("b is broken")!=std::string::npos, "aggregated message incomplete");

            auto t=sid.reportedSupplementQuantities();
            insight::assertion(
                boost::get<supplementedInputDataBase::ReportedError>(&t.at("b").value)!=nullptr,
                "expected error in table" );
            insight::assertion(boost::get<double>(t.at("c").value)==3., "unexpected reported value");
        }

        {
            cout<<"== on demand, without launch"<<endl;
            DiamondSID sid(false);
            auto t=sid.reportedSupplementQuantities();
            insight::assertion(
                boost::get<std::string>(t.at("b").value)=="(computing...)",
                "table must not trigger computation" );
            insight::assertion(fabs(sid.d_-5.)<1e-10, "unexpected value of d");
            insight::assertion(sid.complete()==false, "e not yet computed");
            sid.computeAll();
            insight::assertion(sid.complete(), "expected complete");
        }

        {
            cout<<"== long chain"<<endl;
            int n=4*std::max<int>(1, boost::thread::hardware_concurrency());
            ChainSID sid(n);
            sid.computeAll();
            insight::assertion(sid.complete(), "chain incomplete");
            insight::assertion(sid.q_[n-1]==n-1, "unexpected chain value");
        }

        {
            cout<<"== cancellation"<<endl;
            DiamondSID sid(false);
            sid.launchAll();
            sid.cancelDeferredComputations();
            // whatever was interrupted is reported as error, nothing hangs
            cout<<"errors after cancel: "<<sid.errors().size()<<endl;
        }

        {
            cout<<"== interrupted while waiting"<<endl;
            DiamondSID sid(false);
            boost::thread t([&sid]()
            {
                try { sid.computeAll(); }
                catch (const boost::thread_interrupted&) {}
            });
            boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
            t.interrupt();
            t.join();
            // the workers must still be found and stopped
            sid.cancelDeferredComputations();
            insight::assertion(
                sid.b_.status()!=SupplementedQuantityBase::Running
                && sid.c_.status()!=SupplementedQuantityBase::Running,
                "no computation may run after cancellation" );
        }

        {
            cout<<"== cyclic dependency"<<endl;
            CyclicSID sid;
            auto msg=expectThrow([&]() { sid.throwIfIncomplete(); });
            cout<<msg<<endl;
            insight::assertion(msg.find("cyclic")!=std::string::npos, "expected cycle detection");
        }

        {
            cout<<"== directly assigned value"<<endl;
            Supplemented<double> v(2.);
            insight::assertion(v.ok() && v==2., "direct value");
            Supplemented<double> u;
            insight::assertion(!u.ok(), "undefined value");
            expectThrow([&]() { double x=u; (void)x; });
        }
    }
    catch (const std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }

    return 0;
}
