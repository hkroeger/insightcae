/*
 * Tests the parameter set visualization machinery under (high frequency) parameter changes:
 *
 *   IQParameterSetModel (edited via setData)
 *     -> IQParameterSetVisualizationScheduler (debounce, cancel, relaunch)
 *       -> CADParameterSetModelVisualizer (background thread)
 *         -> IQISCADModelRebuilder
 *           -> IQCADItemModel
 *
 * A probe visualizer records concurrency, reads of the parameters during
 * computation and the value, for which the output was created.
 * The final state of the CAD model is compared with the last parameter value.
 */

#include <QtTest>
#include <QApplication>
#include <QElapsedTimer>

#include <atomic>
#include <mutex>
#include <random>
#include <memory>

#include "boost/thread.hpp"

#include "base/exception.h"
#include "base/exceptionhandling.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/parameters/simpleparameter.h"

#include "cadparametersetvisualizer.h"
#include "iqparametersetvisualizationscheduler.h"
#include "iqparametersetmodel.h"
#include "iqcaditemmodel.h"
#include "cadparameter.h"

#include "test_pdl.h"

using namespace insight;




namespace
{

const std::string valuePath = "ap/0";
const double throwingValue = -1.;
const double throwingDeferredValue = -2.;
const int nFiller = 5;


struct ProbeState
{
    std::atomic<int> active{0}, maxActive{0};
    std::atomic<int> started{0}, interrupted{0}, completed{0};
    std::atomic<int> snapshotViolations{0};

    // configuration
    std::atomic<int> minWorkMs{0}, maxWorkMs{0};
    std::atomic<int> fillerSleepMs{0};
    unsigned seed = 1;

    std::mutex m;
    std::vector<double> completedValues;

    void enter()
    {
        int a = ++active;
        int ma = maxActive;
        while (a>ma && !maxActive.compare_exchange_weak(ma, a)) {}
    }

    void leave()
    {
        --active;
    }
};


struct ActiveGuard
{
    ProbeState& st;
    ActiveGuard(ProbeState& s) : st(s) { st.enter(); }
    ~ActiveGuard() { st.leave(); }
};


double valueOf(const ParameterSet& ps)
{
    return ps.get<DoubleParameter>(valuePath)();
}


std::string parityName(double x)
{
    return (std::lround(x)%2==0) ? "even" : "odd";
}




/**
 * supplemented input data with deferred quantities,
 * one of which fails for the sentinel value throwingDeferredValue
 */
struct ProbeSID
    : public supplementedInputDataBase
{
    Supplemented<double> fine_, broken_;

    ProbeSID(
        ParameterSetInput&& ip,
        const boost::filesystem::path& wd,
        ActionProgress& ap,
        bool breakIt )
        : supplementedInputDataBase(std::move(ip), wd, ap)
    {
        fine_ = deferred("fine", []() { return 1.; });
        broken_ = deferred("broken", [breakIt]() -> double
        {
            if (breakIt)
                throw insight::Exception("probe failure in deferred quantity");
            return 2.;
        });
        reportSupplementQuantity("fine", fine_, "always computable");
        reportSupplementQuantity("broken", broken_, "fails for sentinel value");
    }
};




class ProbeVisualizer
    : public CADParameterSetModelVisualizer
{
    ProbeState& st_;
    double x_ = 0;

public:
    ProbeVisualizer(QObject* parent, IQParameterSetModel* psm, ProbeState& st)
        : CADParameterSetModelVisualizer(parent, psm, "", consoleProgressDisplayer),
          st_(st)
    {}

    std::shared_ptr<supplementedInputDataBase> computeSupplementedInput() override
    {
        ActiveGuard g(st_);
        int runNo = ++st_.started;

        try
        {
            x_ = valueOf(parameters());

            std::mt19937 gen(st_.seed + runNo);
            int minw=st_.minWorkMs, maxw=std::max(int(st_.maxWorkMs), minw);
            int workMs = std::uniform_int_distribution<int>(minw, maxw)(gen);

            // simulate work, check that the parameters do not change meanwhile
            for (int t=0; t<workMs; t+=5)
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
                if (valueOf(parameters())!=x_)
                    ++st_.snapshotViolations;
            }

            if (x_==throwingValue)
                throw insight::Exception("probe failure for sentinel value");

            // no conversion into the static TestPDL::Parameters:
            // would require the property libraries (e.g. brake pads) to be installed.
            // The sid refers to the parameter snapshot, which must stay alive with it.
            return std::make_shared<ProbeSID>(
                ParameterSetInput(this->parameters()),
                this->workDir_,
                *this->progress_.forkNewAction(99, "Process input data"),
                x_==throwingDeferredValue );
        }
        catch (const boost::thread_interrupted&)
        {
            ++st_.interrupted;
            throw;
        }
    }

    void recreateVisualizationElements() override
    {
        ActiveGuard g(st_);
        try
        {
            addPoint("p", vec3(x_, 0, 0));
            addPoint(parityName(x_), vec3(x_, 0, 0));

            for (int i=0; i<nFiller; ++i)
            {
                if (int s=st_.fillerSleepMs)
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(s));
                addPoint("filler_"+std::to_string(i), vec3(x_, i, 0));
            }

            // independent part, which fails for the sentinel value:
            // must not prevent the visualization of the rest
            step("deferred", [this]()
            {
                auto& sid = dynamic_cast<const ProbeSID&>(sidBase());
                double v = sid.fine_ + sid.broken_;
                (void)v;
            });

            ++st_.completed;
            std::lock_guard<std::mutex> l(st_.m);
            st_.completedValues.push_back(x_);
        }
        catch (const boost::thread_interrupted&)
        {
            ++st_.interrupted;
            throw;
        }
    }
};


} // anonymous namespace




class CADParameterSetVisualizerScheduling
    : public QObject
{
    Q_OBJECT

    std::unique_ptr<ProbeState> st_;
    std::unique_ptr<IQParameterSetModel> psm_;
    std::unique_ptr<IQCADItemModel> cadm_;
    std::unique_ptr<IQParameterSetVisualizationScheduler> sched_;

    int errors_ = 0;
    int finishedSignals_ = 0;
    supplementedInputDataBasePtr lastSid_;

    // supplemented input data, which did not match the current parameters on arrival
    int staleSids_ = 0;

    // largest time between two event loop iterations
    QTimer heartbeat_;
    QElapsedTimer sinceBeat_;
    qint64 maxGapMs_ = 0;

    // value timeline of point "p" in the CAD model
    std::vector<double> pTimeline_;


    void setValue(double v)
    {
        auto idx = psm_->indexOfPath(valuePath, IQParameterSetModel::valueCol);
        QVERIFY(idx.isValid());
        QVERIFY(psm_->setData(idx, QString::number(v), Qt::EditRole));
        QCOMPARE(valueOf(psm_->getParameterSet()), v);
    }

    boost::optional<arma::mat> point(const std::string& name) const
    {
        const auto& pts = cadm_->points();
        auto i = pts.find(name);
        if (i==pts.end())
            return boost::none;
        return arma::mat(i->second->value());
    }

    void recordP()
    {
        if (auto p = point("p"))
        {
            double x = (*p)(0);
            if (pTimeline_.empty() || pTimeline_.back()!=x)
                pTimeline_.push_back(x);
        }
    }

    void waitIdle(int timeoutMs = 20000)
    {
        QTRY_VERIFY_WITH_TIMEOUT(sched_->isIdle(), timeoutMs);
        // let remaining queued events (e.g. deferred deletes) be processed
        QTest::qWait(20);
        QVERIFY(sched_->isIdle());
    }

    /**
     * checks, that the CAD model contains exactly the output of a computation for value x
     */
    void verifyModelShows(double x)
    {
        auto p = point("p");
        QVERIFY2(bool(p), "point p missing");
        QCOMPARE((*p)(0), x);

        auto par = parityName(x);
        auto other = par=="even" ? "odd" : "even";
        QVERIFY2(bool(point(par)), "parity marker missing");
        QVERIFY2(!point(other), "stale parity marker of previous computation present");

        for (int i=0; i<nFiller; ++i)
        {
            auto f = point("filler_"+std::to_string(i));
            QVERIFY2(bool(f), "filler missing");
            QCOMPARE((*f)(0), x);
        }

        QCOMPARE(int(cadm_->points().size()), 2+nFiller);
    }

    static QString describe(std::exception_ptr ex)
    {
        try
        {
            std::rethrow_exception(ex);
        }
        catch (...)
        {
            auto desc = insight::describeCurrentException();
            return QString::fromStdString(
                std::string(*desc) + "\n" + desc->context_ + "\n" + desc->errorDetails_ );
        }
    }

    void verifyLastSid(double x)
    {
        QVERIFY(bool(lastSid_));
        QCOMPARE(valueOf(lastSid_->parameters()), x);
    }

private Q_SLOTS:

    void init()
    {
        st_ = std::make_unique<ProbeState>();
        psm_ = std::make_unique<IQParameterSetModel>(TestPDL::defaultParameters());
        cadm_ = std::make_unique<IQCADItemModel>();

        errors_ = 0;
        finishedSignals_ = 0;
        staleSids_ = 0;
        lastSid_.reset();
        pTimeline_.clear();

        auto *st = st_.get();
        auto *psm = psm_.get();
        sched_ = std::make_unique<IQParameterSetVisualizationScheduler>(
            [st,psm](QObject* parent) -> CADParameterSetVisualizerGenerator*
            {
                return new ProbeVisualizer(parent, psm, *st);
            },
            cadm_.get(),
            100 );

        connectParameterSetChanged(
            psm_.get(),
            sched_.get(), &IQParameterSetVisualizationScheduler::requestUpdate );

        connect(sched_.get(), &IQParameterSetVisualizationScheduler::visualizationComputationError,
                this, [this](std::exception_ptr ex)
                {
                    ++errors_;
                    qWarning().noquote() << "visualization error:" << describe(ex);
                } );
        connect(sched_.get(), &IQParameterSetVisualizationScheduler::visualizationCalculationFinished,
                this, [this](bool) { ++finishedSignals_; } );
        connect(sched_.get(), &IQParameterSetVisualizationScheduler::updateSupplementedInputData,
                this, [this](supplementedInputDataBasePtr sid)
                {
                    lastSid_=sid;
                    // since any change cancels the running computation,
                    // an arriving sid must always belong to the current parameters
                    if (valueOf(sid->parameters())!=valueOf(psm_->getParameterSet()))
                        ++staleSids_;
                } );

        connect(cadm_.get(), &QAbstractItemModel::dataChanged, this, [this]{ recordP(); });
        connect(cadm_.get(), &QAbstractItemModel::rowsInserted, this, [this]{ recordP(); });

        maxGapMs_ = 0;
        sinceBeat_.start();
        heartbeat_.setInterval(10);
        connect(&heartbeat_, &QTimer::timeout, this, [this]
                {
                    maxGapMs_ = std::max(maxGapMs_, sinceBeat_.restart());
                });
        heartbeat_.start();
    }

    void cleanup()
    {
        heartbeat_.stop();
        heartbeat_.disconnect();
        sched_.reset();
        cadm_.reset();
        psm_.reset();
        st_.reset();
    }


    /**
     * prerequisite (no threads involved):
     * the parameter snapshot, which the visualizer thread works on,
     * must be an independent copy of the parameter set
     */
    void snapshotIsIndependentCopy()
    {
        auto snap = psm_->getParameterSet().cloneAs<ParameterSet>();
        QVERIFY(bool(snap));
        QCOMPARE(valueOf(*snap), valueOf(psm_->getParameterSet()));

        double orgValue = valueOf(*snap);
        setValue(orgValue+1.);
        QCOMPARE(valueOf(*snap), orgValue);

        ParameterSetInput psi(*snap);
        QVERIFY(psi.hasParameterSet());
        QCOMPARE(&psi.parameterSet(), snap.get());
    }


    /**
     * changes within the debounce interval must result in one computation
     */
    void debounceCoalesces()
    {
        for (int i=1; i<=20; ++i)
        {
            setValue(i);
            QTest::qWait(10);
        }
        waitIdle();

        QCOMPARE(sched_->launchedCount(), 1);
        QCOMPARE(int(st_->started), 1);
        verifyModelShows(20);
        verifyLastSid(20);
        QCOMPARE(errors_, 0);
    }


    /**
     * a change during the computation cancels it without error,
     * its result never appears
     */
    void cancelDuringCompute()
    {
        st_->minWorkMs = st_->maxWorkMs = 500;

        setValue(1);
        QTRY_COMPARE_WITH_TIMEOUT(int(st_->started), 1, 2000);
        QTest::qWait(50);

        setValue(2);
        waitIdle();

        QCOMPARE(int(st_->interrupted), 1);
        QCOMPARE(int(st_->started), 2);
        QCOMPARE(errors_, 0);
        QCOMPARE(finishedSignals_, 1);
        QCOMPARE(pTimeline_, std::vector<double>{2});
        verifyModelShows(2);
        verifyLastSid(2);

        // the GUI must not have been blocked by the cancellation
        QVERIFY2(maxGapMs_ < 100, qPrintable(QString("event loop blocked for %1 ms").arg(maxGapMs_)));
    }


    /**
     * cancel, while entities are being emitted.
     * None of the entities of the cancelled computation must remain.
     */
    void cancelDuringIncrementalAdd()
    {
        // first a complete computation
        setValue(1);
        waitIdle();
        verifyModelShows(1);

        st_->fillerSleepMs = 100;
        setValue(2);
        // wait until the second computation has emitted some entities
        QTRY_VERIFY_WITH_TIMEOUT(
            point("p") && (*point("p"))(0)==2., 5000 );

        st_->fillerSleepMs = 0;
        setValue(3);
        waitIdle();

        QVERIFY(st_->interrupted>=1);
        QCOMPARE(errors_, 0);
        verifyModelShows(3);
        verifyLastSid(3);
    }


    /**
     * random changes in high frequency, including structural changes
     */
    void highFrequencyStress()
    {
        unsigned seed = 4711;
        if (auto s=qgetenv("INSIGHT_TEST_SEED"); !s.isEmpty())
            seed=s.toUInt();
        qInfo() << "seed:" << seed;

        st_->seed = seed;
        st_->maxWorkMs = 300;
        st_->fillerSleepMs = 2;

        std::mt19937 gen(seed);
        std::uniform_int_distribution<int> interval(0, 250);
        std::uniform_int_distribution<int> action(0, 19);

        const int nChanges = 500;
        double value = 0;
        int nArrayChanges = 0;

        // emulate a run start (as in the workbench): wait for idle, then take the sid
        int idleCallbacks = 0, badIdleSids = 0;
        auto runStart = [&]()
        {
            ++idleCallbacks;
            auto sid = sched_->upToDateSupplementedInputData();
            if (!sid || valueOf(sid->parameters())!=valueOf(psm_->getParameterSet()))
                ++badIdleSids;
        };

        QElapsedTimer total; total.start();
        for (int i=0; i<nChanges; ++i)
        {
            int a = action(gen);
            if (a==0)
            {
                // structural change: append an element, then remove it again
                auto api = psm_->indexOfPath("ap", 0);
                psm_->appendArrayElement(api, DoubleParameter(0., ""));
                ++nArrayChanges;
            }
            else if (a==1 && nArrayChanges>0)
            {
                auto api = psm_->indexOfPath("ap", 0);
                psm_->removeArrayElement(
                    psm_->index(psm_->rowCount(api)-1, 0, api) );
                --nArrayChanges;
            }
            else
            {
                setValue(++value);
            }

            // right after a change, there must be no up-to-date sid
            QVERIFY(!sched_->upToDateSupplementedInputData());

            if (a==2)
                sched_->whenIdle(runStart);

            QTest::qWait(interval(gen));
        }
        qInfo() << nChanges << "changes in" << total.elapsed() << "ms,"
                << sched_->launchedCount() << "launched,"
                << int(st_->interrupted) << "interrupted,"
                << int(st_->completed) << "completed";

        waitIdle();

        QCOMPARE(int(st_->maxActive), 1);
        QCOMPARE(int(st_->snapshotViolations), 0);
        QCOMPARE(errors_, 0);
        QCOMPARE(staleSids_, 0);
        qInfo() << idleCallbacks << "emulated run starts";
        QVERIFY(idleCallbacks>0);
        QCOMPARE(badIdleSids, 0);

        verifyModelShows(value);
        verifyLastSid(value);

        // debouncing was effective
        QVERIFY(sched_->launchedCount() < nChanges);

        // last completed computation was the one for the final value
        {
            std::lock_guard<std::mutex> l(st_->m);
            QVERIFY(!st_->completedValues.empty());
            QCOMPARE(st_->completedValues.back(), value);
        }

        QVERIFY2(maxGapMs_ < 100, qPrintable(QString("event loop blocked for %1 ms").arg(maxGapMs_)));
    }


    /**
     * destruction during a running computation must neither crash nor hang
     */
    void destroyWhileRunning()
    {
        st_->minWorkMs = st_->maxWorkMs = 2000;

        setValue(1);
        QTRY_COMPARE_WITH_TIMEOUT(int(st_->started), 1, 2000);

        QElapsedTimer t; t.start();
        sched_.reset();
        QVERIFY2(t.elapsed() < 500, "destruction had to wait for the computation to complete");

        cadm_.reset();
        psm_.reset();
        QTest::qWait(50);
    }


    /**
     * errors of the current computation are reported, the next change recovers
     */
    void errorPathReported()
    {
        setValue(throwingValue);
        waitIdle();
        QCOMPARE(errors_, 1);

        setValue(5);
        waitIdle();
        QCOMPARE(errors_, 1);
        verifyModelShows(5);
        verifyLastSid(5);
    }

    /**
     * failure in the supplemented input data:
     * the parts of the visualization, which do not depend on it, are shown nevertheless
     */
    void partialVisualizationOnInputDataError()
    {
        setValue(throwingValue);
        waitIdle();
        QCOMPARE(errors_, 1);
        QCOMPARE(finishedSignals_, 0);
        verifyModelShows(throwingValue);
        QVERIFY(!lastSid_);
        QVERIFY(!sched_->upToDateSupplementedInputData());
    }

    /**
     * failure in a deferred quantity:
     * the complete visualization is shown, the error is reported once,
     * the sid is delivered (for the table) but never used for a run
     */
    void partialVisualizationOnDeferredError()
    {
        setValue(throwingDeferredValue);
        waitIdle();
        QCOMPARE(errors_, 1);
        QCOMPARE(finishedSignals_, 0);
        verifyModelShows(throwingDeferredValue);
        verifyLastSid(throwingDeferredValue);
        QVERIFY(!lastSid_->complete());
        QCOMPARE(int(lastSid_->rootErrors().size()), 1);
        QVERIFY(!sched_->upToDateSupplementedInputData());

        auto t=lastSid_->reportedSupplementQuantities();
        QVERIFY(boost::get<supplementedInputDataBase::ReportedError>(&t.at("broken").value));
        QCOMPARE(boost::get<double>(t.at("fine").value), 1.);

        // recovers
        setValue(4);
        waitIdle();
        QCOMPARE(errors_, 1);
        verifyModelShows(4);
        verifyLastSid(4);
        QVERIFY(lastSid_->complete());
        QVERIFY(bool(sched_->upToDateSupplementedInputData()));
    }

    /**
     * a run start right after a change waits for the visualization
     * and gets the supplemented input data of the new parameters
     */
    void whenIdleWaitsForCurrentParameters()
    {
        st_->minWorkMs = st_->maxWorkMs = 300;

        setValue(1);
        int calls=0;
        supplementedInputDataBasePtr sid;
        bool modelShowsValue=false;
        sched_->whenIdle([&]()
        {
            ++calls;
            sid=sched_->upToDateSupplementedInputData();
            auto p=point("p");
            modelShowsValue = p && (*p)(0)==1.;
        });
        QCOMPARE(calls, 0);

        QTRY_COMPARE_WITH_TIMEOUT(calls, 1, 5000);
        QVERIFY(bool(sid));
        QCOMPARE(valueOf(sid->parameters()), 1.);
        QVERIFY(modelShowsValue);

        QTest::qWait(200);
        QCOMPARE(calls, 1);
    }


    /**
     * changes while waiting: the callback waits for the latest parameters
     */
    void whenIdleFollowsEditsWhileWaiting()
    {
        st_->minWorkMs = st_->maxWorkMs = 300;

        setValue(1);
        int calls=0;
        supplementedInputDataBasePtr sid;
        sched_->whenIdle([&]()
        {
            ++calls;
            sid=sched_->upToDateSupplementedInputData();
        });

        QTest::qWait(50);
        QCOMPARE(calls, 0);
        setValue(2);

        QTRY_COMPARE_WITH_TIMEOUT(calls, 1, 5000);
        QVERIFY(bool(sid));
        QCOMPARE(valueOf(sid->parameters()), 2.);
        QCOMPARE(errors_, 0);
    }


    /**
     * nothing pending: callback is invoked immediately
     */
    void whenIdleImmediateWhenIdle()
    {
        setValue(4);
        waitIdle();

        int calls=0;
        sched_->whenIdle([&]() { ++calls; });
        QCOMPARE(calls, 1);

        auto sid=sched_->upToDateSupplementedInputData();
        QVERIFY(bool(sid));
        QCOMPARE(valueOf(sid->parameters()), 4.);
    }


    /**
     * the debounce interval is skipped, when waiting for idle
     */
    void whenIdleSkipsDebounce()
    {
        setValue(3);

        QElapsedTimer t; t.start();
        int calls=0;
        sched_->whenIdle([&]() { ++calls; });
        QTRY_COMPARE_WITH_TIMEOUT(calls, 1, 2000);
        QVERIFY2(t.elapsed() < 80,
                 qPrintable(QString("callback after %1 ms").arg(t.elapsed())));
    }


    /**
     * failed preprocessing: no sid (run has to compute it itself)
     */
    void failedComputeGivesNoSid()
    {
        setValue(throwingValue);

        int calls=0;
        supplementedInputDataBasePtr sid;
        sched_->whenIdle([&]()
        {
            ++calls;
            sid=sched_->upToDateSupplementedInputData();
        });
        QTRY_COMPARE_WITH_TIMEOUT(calls, 1, 5000);
        QVERIFY(!sid);
        QCOMPARE(errors_, 1);
    }


    /**
     * a cancelled wait never invokes the callback
     */
    void cancelWhenIdle()
    {
        st_->minWorkMs = st_->maxWorkMs = 300;

        setValue(1);
        int calls=0;
        sched_->whenIdle([&]() { ++calls; });
        sched_->cancelWhenIdle();

        waitIdle();
        QTest::qWait(50);
        QCOMPARE(calls, 0);
    }
};




QTEST_MAIN(CADParameterSetVisualizerScheduling)

#include "CADParameterSetVisualizerScheduling.moc"
