#include "base/supplementedinputdata.h"
#include "base/exception.h"
#include "base/progressdisplayer.h"
#include "base/warningdispatcher.h"

#include <iostream>
#include <atomic>
#include <mutex>

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


/**
 * issues warnings in deferred computations, one of which fails
 */
struct WarningSID
    : public supplementedInputDataBase
{
    Supplemented<double> warns_, fails_, fine_;

    WarningSID()
        : supplementedInputDataBase(boost::filesystem::path("."))
    {
        warns_ = deferred("warns", []()
        {
            insight::Warning("value is questionable");
            return 1.;
        });
        fails_ = deferred("fails", []() -> double
        {
            insight::Warning("about to fail");
            throw insight::Exception("broken input");
        });
        fine_ = deferred("fine", []() { return 2.; });
    }
};


/**
 * records, which progress displays were created and finished
 */
struct RecordingDisplayer
    : public ProgressDisplayer
{
    std::mutex m;
    std::vector<std::string> created; // in order of creation
    std::set<std::string> finished;
    std::map<std::string, double> lastValue;

    void setActionProgressValue(const std::string& path, double value) override
    {
        std::lock_guard<std::mutex> l(m);
        if (!lastValue.count(path)) created.push_back(path);
        lastValue[path]=value;
    }
    void setMessageText(const std::string&, const std::string&) override {}
    void finishActionProgress(const std::string& path) override
    {
        std::lock_guard<std::mutex> l(m);
        finished.insert(path);
    }
    void reset() override {}
    void update(const ProgressState&) override {}
    void logMessage(const std::string&) override {}

    bool wasCreated(const std::string& path)
    {
        std::lock_guard<std::mutex> l(m);
        return std::find(created.begin(), created.end(), path)!=created.end();
    }

    bool allFinished()
    {
        std::lock_guard<std::mutex> l(m);
        for (const auto& c: created)
            if (!finished.count(c)) return false;
        return true;
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
            cout<<"== lazy progress actions"<<endl;
            RecordingDisplayer rd;
            {
                auto root=rd.forkNewAction(1, "root");
                insight::assertion(rd.wasCreated("root"), "root action has to be displayed");

                {
                    auto unused=root->forkNewLazyAction(1, "unused");
                }
                insight::assertion(!rd.wasCreated("root/unused"), "unused lazy action must not be displayed");

                auto lazy=root->forkNewLazyAction(1, "lazy");
                auto inner=lazy->forkNewLazyAction(1, "inner");
                insight::assertion(!rd.wasCreated("root/lazy"), "lazy action displayed before first report");

                inner->message("working");
                insight::assertion(
                    rd.wasCreated("root/lazy") && rd.wasCreated("root/lazy/inner"),
                    "first report has to start the action and its lazy parents" );
                {
                    std::lock_guard<std::mutex> l(rd.m);
                    auto ip=std::find(rd.created.begin(), rd.created.end(), "root/lazy");
                    auto ic=std::find(rd.created.begin(), rd.created.end(), "root/lazy/inner");
                    insight::assertion(ip<ic, "parent has to be displayed before child");
                }
                inner.reset();
                lazy.reset();

                // concurrent forking and destruction of children of the same parent
                std::vector<std::unique_ptr<boost::thread> > threads;
                for (int t=0; t<20; ++t)
                {
                    threads.push_back(std::make_unique<boost::thread>([root,t]()
                    {
                        for (int k=0; k<200; ++k)
                        {
                            auto c=root->forkNewLazyAction(1, "t"+std::to_string(t));
                            if (k%2) c->stepUp();
                        }
                    }));
                }
                for (auto& t: threads) t->join();
            }
            insight::assertion(rd.allFinished(), "all displayed actions have to be finished");
        }

        {
            cout<<"== progress display of deferred quantities"<<endl;
            RecordingDisplayer rd;
            {
                DiamondSID sid(false);
                sid.computeAll(rd.forkNewAction(1, "input"));
            }
            std::set<std::string> created(rd.created.begin(), rd.created.end());
            for (const auto& c: created) cout<<"  displayed: "<<c<<endl;
            insight::assertion(
                created==std::set<std::string>({"input", "input/c"}),
                "only the overall progress and quantities reporting progress themselves"
                " shall be displayed (got %d displays)", int(created.size()) );
            insight::assertion(
                fabs(rd.lastValue.at("input")-1.)<1e-10,
                "overall progress has to be completed (is %g)", rd.lastValue.at("input") );
            insight::assertion(rd.allFinished(), "all displayed actions have to be finished");
        }

        {
            cout<<"== warnings and errors of the input data"<<endl;

            // callback on the dispatcher of this thread (top of the chain, like in the GUI)
            std::mutex m;
            std::vector<std::pair<std::string,bool> > seen; // message, recorded
            int cbid = WarningDispatcher::getCurrent().addIssueCallback(
                [&](const insight::Exception& w)
                {
                    std::lock_guard<std::mutex> l(m);
                    seen.push_back({w.message(), WarningDispatcher::currentIssueIsRecorded()});
                });

            {
                WarningSID sid;
                sid.computeAll();
                for (const auto& i: sid.issues())
                    cout<<"  "<<(i.severity==InputDataIssue::Error?"E ":"W ")<<i.source<<": "<<i.message<<endl;

                auto iss=sid.issues();
                auto has=[&](InputDataIssue::Severity sev, const std::string& src, const std::string& msg)
                {
                    return std::any_of(iss.begin(), iss.end(), [&](const InputDataIssue& i)
                    { return i.severity==sev && i.source==src && i.message.find(msg)!=std::string::npos; });
                };
                insight::assertion(iss.size()==3, "expected 3 issues, got %d", int(iss.size()));
                insight::assertion(has(InputDataIssue::Warning, "warns", "questionable"), "warning of quantity missing");
                insight::assertion(has(InputDataIssue::Warning, "fails", "about to fail"), "warning of failing quantity missing");
                insight::assertion(has(InputDataIssue::Error, "fails", "broken input"), "error missing");

                sid.recordWarning("input data", "explicit");
                insight::assertion(sid.issues().size()==4, "explicitly recorded warning missing");
            }

            // a warning outside of any recorder
            insight::Warning("unrelated warning");

            WarningDispatcher::getCurrent().removeIssueCallback(cbid);

            std::lock_guard<std::mutex> l(m);
            insight::assertion(seen.size()==3, "all warnings have to be dispatched as usual (got %d)", int(seen.size()));
            for (const auto& w: seen)
            {
                bool shouldBeRecorded = w.first!="unrelated warning";
                insight::assertion(
                    w.second==shouldBeRecorded,
                    "wrong recorded flag for warning \"%s\"", w.first.c_str() );
            }

            // nested recorders: only the innermost one receives
            std::vector<std::string> outer, inner;
            {
                ScopedWarningRecorder ro([&](const insight::Exception& w) { outer.push_back(w.message()); });
                insight::Warning("to outer");
                {
                    ScopedWarningRecorder ri([&](const insight::Exception& w) { inner.push_back(w.message()); });
                    insight::Warning("to inner");
                }
                insight::Warning("to outer again");
            }
            insight::assertion(
                outer==std::vector<std::string>({"to outer", "to outer again"})
                && inner==std::vector<std::string>({"to inner"}),
                "nested recorders" );
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
