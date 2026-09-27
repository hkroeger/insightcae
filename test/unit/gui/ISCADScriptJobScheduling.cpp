/*
 * Tests the background job scheduling of ISCAD:
 *
 *   IQDebouncedJobScheduler (debounce, cancel, relaunch, protected explicit jobs)
 *     -> probe jobs (background thread)
 *
 *   IQISCADScriptJobScheduler (background parse, reuse of parse result in rebuild)
 *     -> IQISCADScriptJob (background thread)
 *       -> IQISCADModelRebuilder
 *         -> IQCADItemModel
 */

#include <QtTest>
#include <QApplication>

#include <atomic>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>

#include "base/exception.h"

#include "iqdebouncedjobscheduler.h"
#include "iqiscadscriptjobscheduler.h"
#include "iqcaditemmodel.h"
#include "cadparameters.h"

using namespace insight;


namespace
{




struct ProbeState
{
    std::atomic<int> active{0}, maxActive{0};
    std::atomic<int> started{0}, cancelled{0}, completed{0};

    std::vector<std::string> completedLabels; // GUI thread only

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




/**
 * a job, which works for a given time in a background thread
 */
class ProbeJob
    : public QObject
{
    Q_OBJECT

    ProbeState& st_;
    std::string label_;
    int workMs_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> cancelled_{false}, active_{false};

public:
    ProbeJob(ProbeState& st, const std::string& label, int workMs, QObject* parent)
        : QObject(parent), st_(st), label_(label), workMs_(workMs)
    {
        // queued, since emitted from the background thread
        connect(this, &ProbeJob::threadEnded, this, [this]()
                {
                    if (thread_) { thread_->join(); thread_.reset(); }
                });
    }

    ~ProbeJob()
    {
        stopAndWait();
    }

    const std::string& label() const { return label_; }

    void launch()
    {
        active_=true;
        thread_=std::make_unique<std::thread>([this]()
        {
            st_.enter();
            ++st_.started;
            bool ok=true;
            for (int t=0; t<workMs_; t+=5)
            {
                if (cancelled_) { ok=false; break; }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            if (ok && !cancelled_)
            {
                ++st_.completed;
                Q_EMIT completed(QString::fromStdString(label_));
            }
            else
            {
                ++st_.cancelled;
            }
            st_.leave();
            active_=false;
            Q_EMIT threadEnded();
        });
    }

    void cancelAndDeleteLater()
    {
        cancelled_=true;
        connect(this, &ProbeJob::threadEnded, this, &QObject::deleteLater);
        if (!active_)
            deleteLater();
    }

    void stopAndWait()
    {
        cancelled_=true;
        if (thread_) { thread_->join(); thread_.reset(); }
    }

Q_SIGNALS:
    void completed(QString label);
    void threadEnded();
};




class ProbeScheduler
    : public IQDebouncedJobScheduler
{
    ProbeState& st_;

public:
    std::string nextLabel = "debounced";
    int debouncedWorkMs = 0;

    ProbeScheduler(ProbeState& st, int debounceMs)
        : IQDebouncedJobScheduler(debounceMs), st_(st)
    {}

    ~ProbeScheduler()
    {
        shutdown();
    }

    ProbeJob* setup(quint64 id, ProbeJob* job)
    {
        connect(job, &ProbeJob::completed, this, [this,id](QString label)
                {
                    if (isCurrent(id))
                        st_.completedLabels.push_back(label.toStdString());
                });
        connect(job, &ProbeJob::threadEnded, this, [this,id]() { jobThreadEnded(id); });
        return job;
    }

    void launchExplicit(const std::string& label, int workMs, bool cancelOnUpdate=false)
    {
        launchNow(
            [this,label,workMs](quint64 id)
            {
                return setup(id, new ProbeJob(st_, label, workMs, this));
            },
            cancelOnUpdate );
    }

protected:
    QObject* createDebouncedJob(quint64 id) override
    {
        return setup(id, new ProbeJob(st_, nextLabel, debouncedWorkMs, this));
    }

    void startJob(QObject* job) override
    {
        static_cast<ProbeJob*>(job)->launch();
    }

    void cancelJobAndDeleteLater(QObject* job) override
    {
        static_cast<ProbeJob*>(job)->cancelAndDeleteLater();
    }

    void stopJobAndWait(QObject* job) override
    {
        static_cast<ProbeJob*>(job)->stopAndWait();
    }
};




} // anonymous namespace




class ISCADScriptJobScheduling
    : public QObject
{
    Q_OBJECT

    // generic scheduler
    std::unique_ptr<ProbeState> st_;
    std::unique_ptr<ProbeScheduler> ps_;

    // ISCAD scheduler
    std::string script_;
    std::unique_ptr<IQCADItemModel> cadm_;
    std::unique_ptr<IQISCADScriptJobScheduler> iss_;

    struct StartedJob { IQISCADScriptModelGenerator::Task task; bool reused; };
    std::vector<StartedJob> startedJobs_;
    int errors_ = 0;
    long lastErrorPos_ = -2;
    IQISCADScriptModelGenerator::Task lastErrorTask_ = IQISCADScriptModelGenerator::Parse;
    int rebuilt_ = 0;


    void waitIdle(IQDebouncedJobScheduler& s, int timeoutMs = 20000)
    {
        QTRY_VERIFY_WITH_TIMEOUT(s.isIdle(), timeoutMs);
        // let remaining queued events (e.g. deferred deletes) be processed
        QTest::qWait(20);
        QVERIFY(s.isIdle());
    }

    void setScript(const std::string& s)
    {
        script_=s;
        iss_->requestUpdate();
    }

    int countStarted(IQISCADScriptModelGenerator::Task task, bool reused) const
    {
        int n=0;
        for (const auto& j: startedJobs_)
            if (j.task==task && j.reused==reused) ++n;
        return n;
    }

    boost::optional<double> scalar(const std::string& name) const
    {
        const auto& s = cadm_->scalars();
        auto i = s.find(name);
        if (i==s.end())
            return boost::none;
        return i->second->value();
    }


private Q_SLOTS:

    void init()
    {
        st_ = std::make_unique<ProbeState>();
        ps_ = std::make_unique<ProbeScheduler>(*st_, 50);

        script_.clear();
        cadm_ = std::make_unique<IQCADItemModel>();
        iss_ = std::make_unique<IQISCADScriptJobScheduler>(
            [this]() { return script_; },
            cadm_.get(),
            50 );

        startedJobs_.clear();
        errors_ = 0;
        lastErrorPos_ = -2;
        rebuilt_ = 0;

        connect(iss_.get(), &IQISCADScriptJobScheduler::jobStarted,
                this, [this](IQISCADScriptModelGenerator::Task t, bool reused)
                {
                    startedJobs_.push_back({t, reused});
                });
        connect(iss_.get(), &IQISCADScriptJobScheduler::scriptError,
                this, [this](long failpos, QString msg, int, IQISCADScriptModelGenerator::Task t)
                {
                    ++errors_;
                    lastErrorPos_=failpos;
                    lastErrorTask_=t;
                    qDebug().noquote() << "script error:" << msg;
                });
        connect(iss_.get(), &IQISCADScriptJobScheduler::modelRebuilt,
                this, [this]() { ++rebuilt_; });
    }

    void cleanup()
    {
        iss_.reset();
        cadm_.reset();
        ps_.reset();
        st_.reset();
    }


    // ======== generic scheduler

    /**
     * changes within the debounce interval must result in one job
     */
    void debounceCoalesces()
    {
        for (int i=0; i<20; ++i)
        {
            ps_->requestUpdate();
            QTest::qWait(5);
        }
        waitIdle(*ps_);

        QCOMPARE(ps_->launchedCount(), 1);
        QCOMPARE(int(st_->completed), 1);
    }


    /**
     * a change during a debounced job cancels it, its output never appears
     */
    void updateCancelsDebouncedJob()
    {
        ps_->debouncedWorkMs=500;
        ps_->nextLabel="first";
        ps_->requestUpdate();
        QTRY_COMPARE_WITH_TIMEOUT(int(st_->started), 1, 2000);

        ps_->nextLabel="second";
        ps_->requestUpdate();
        waitIdle(*ps_);

        QCOMPARE(int(st_->cancelled), 1);
        QCOMPARE(int(st_->completed), 1);
        QCOMPARE(st_->completedLabels, std::vector<std::string>{"second"});
        QCOMPARE(int(st_->maxActive), 1);
    }


    /**
     * a protected explicit job is not cancelled by changes.
     * The debounced job is launched after it has ended.
     */
    void protectedJobSurvivesUpdates()
    {
        ps_->launchExplicit("explicit", 400, false);
        QTRY_COMPARE_WITH_TIMEOUT(int(st_->started), 1, 2000);

        for (int i=0; i<5; ++i)
        {
            ps_->requestUpdate();
            QTest::qWait(10);
        }
        waitIdle(*ps_);

        QCOMPARE(int(st_->cancelled), 0);
        QCOMPARE(st_->completedLabels, (std::vector<std::string>{"explicit", "debounced"}));
        QCOMPARE(ps_->launchedCount(), 2);
        QCOMPARE(int(st_->maxActive), 1);
    }


    /**
     * an unprotected explicit job is cancelled by changes
     */
    void unprotectedExplicitJobIsCancelled()
    {
        ps_->launchExplicit("explicit", 400, true);
        QTRY_COMPARE_WITH_TIMEOUT(int(st_->started), 1, 2000);

        ps_->requestUpdate();
        waitIdle(*ps_);

        QCOMPARE(int(st_->cancelled), 1);
        QCOMPARE(st_->completedLabels, std::vector<std::string>{"debounced"});
        QCOMPARE(int(st_->maxActive), 1);
    }


    /**
     * an explicit launch cancels a running debounced job, which is relaunched afterwards
     */
    void explicitLaunchPreemptsDebouncedJob()
    {
        ps_->debouncedWorkMs=400;
        ps_->requestUpdate();
        QTRY_COMPARE_WITH_TIMEOUT(int(st_->started), 1, 2000);

        ps_->launchExplicit("explicit", 50);
        waitIdle(*ps_);

        QCOMPARE(int(st_->cancelled), 1);
        QCOMPARE(st_->completedLabels, (std::vector<std::string>{"explicit", "debounced"}));
        QCOMPARE(int(st_->maxActive), 1);
    }


    /**
     * whenIdle skips the debounce interval and fires after the job has completed
     */
    void whenIdleWaitsForJob()
    {
        ps_->debouncedWorkMs=100;
        ps_->requestUpdate();

        bool called=false;
        ps_->whenIdle([&]()
                      {
                          called=true;
                          QCOMPARE(int(st_->completed), 1);
                      });
        QVERIFY(!called);
        waitIdle(*ps_);
        QVERIFY(called);
    }


    /**
     * cancel stops everything, nothing is launched afterwards
     */
    void cancelStopsEverything()
    {
        ps_->launchExplicit("explicit", 400);
        QTRY_COMPARE_WITH_TIMEOUT(int(st_->started), 1, 2000);
        ps_->requestUpdate();

        ps_->cancel();
        waitIdle(*ps_);
        QTest::qWait(100);

        QCOMPARE(int(st_->cancelled), 1);
        QCOMPARE(int(st_->completed), 0);
        QCOMPARE(ps_->launchedCount(), 1);
    }


    // ======== ISCAD scheduler

    /**
     * the explicit rebuild reuses the result of the background parser
     */
    void rebuildReusesBackgroundParse()
    {
        setScript("a=1;\nb=2*a;\n");
        waitIdle(*iss_);

        QCOMPARE(countStarted(IQISCADScriptModelGenerator::Parse, false), 1);
        QVERIFY(bool(iss_->lastParseResult()));
        QCOMPARE(iss_->lastParseResult()->script, script_);
        QVERIFY(!scalar("a")); // parsing does not modify the model

        iss_->rebuild(IQISCADScriptModelGenerator::Rebuild);
        waitIdle(*iss_);

        QCOMPARE(int(startedJobs_.size()), 2);
        QCOMPARE(countStarted(IQISCADScriptModelGenerator::Rebuild, true), 1);
        QCOMPARE(errors_, 0);
        QCOMPARE(rebuilt_, 1);
        QCOMPARE(*scalar("a"), 1.);
        QCOMPARE(*scalar("b"), 2.);
    }


    /**
     * a rebuild requested during the debounce interval
     * waits for the background parse and reuses it
     */
    void rebuildWaitsForBackgroundParse()
    {
        setScript("a=3;\n");
        iss_->rebuild(IQISCADScriptModelGenerator::Rebuild);
        waitIdle(*iss_);

        QCOMPARE(int(startedJobs_.size()), 2);
        QCOMPARE(countStarted(IQISCADScriptModelGenerator::Parse, false), 1);
        QCOMPARE(countStarted(IQISCADScriptModelGenerator::Rebuild, true), 1);
        QCOMPARE(*scalar("a"), 3.);
    }


    /**
     * after a change, the rebuild does not use the outdated parse result
     */
    void rebuildAfterChangeUsesNewScript()
    {
        setScript("a=1;\n");
        waitIdle(*iss_);
        iss_->rebuild(IQISCADScriptModelGenerator::Rebuild);
        waitIdle(*iss_);
        QCOMPARE(*scalar("a"), 1.);

        setScript("a=5;\nc=a+1;\n");
        iss_->rebuild(IQISCADScriptModelGenerator::Rebuild);
        waitIdle(*iss_);

        QCOMPARE(*scalar("a"), 5.);
        QCOMPARE(*scalar("c"), 6.);
        QCOMPARE(errors_, 0);
    }


    /**
     * without background parsing, the rebuild parses by itself
     */
    void rebuildWithoutBackgroundParsing()
    {
        iss_->setBackgroundParsingEnabled(false);
        setScript("a=7;\n");
        QTest::qWait(150);
        QVERIFY(startedJobs_.empty());

        iss_->rebuild(IQISCADScriptModelGenerator::Rebuild);
        waitIdle(*iss_);

        QCOMPARE(int(startedJobs_.size()), 1);
        QCOMPARE(countStarted(IQISCADScriptModelGenerator::Rebuild, false), 1);
        QCOMPARE(*scalar("a"), 7.);
    }


    /**
     * rebuild of a given script (up to cursor): symbols beyond are removed
     */
    void rebuildScriptUsesGivenScript()
    {
        setScript("a=1;\nb=2;\n");
        iss_->rebuild(IQISCADScriptModelGenerator::Rebuild);
        waitIdle(*iss_);
        QVERIFY(bool(scalar("b")));

        iss_->rebuildScript("a=1;\n", IQISCADScriptModelGenerator::Rebuild);
        waitIdle(*iss_);

        QCOMPARE(*scalar("a"), 1.);
        QVERIFY(!scalar("b"));
        QCOMPARE(countStarted(IQISCADScriptModelGenerator::Rebuild, false), 1);
    }


    /**
     * a script error is reported without location during background parsing,
     * with location on rebuild
     */
    void scriptErrorReporting()
    {
        setScript("a=1;\nb=;\n");
        waitIdle(*iss_);

        QCOMPARE(errors_, 1);
        QCOMPARE(lastErrorPos_, -1L);
        QCOMPARE(lastErrorTask_, IQISCADScriptModelGenerator::Parse);

        iss_->rebuild(IQISCADScriptModelGenerator::Rebuild);
        waitIdle(*iss_);

        QCOMPARE(errors_, 2);
        QVERIFY(lastErrorPos_>=0);
        QCOMPARE(lastErrorTask_, IQISCADScriptModelGenerator::Rebuild);
        QCOMPARE(countStarted(IQISCADScriptModelGenerator::Rebuild, true), 1);
    }


    /**
     * changes during an explicit rebuild do not cancel it
     */
    void changeDuringRebuildDoesNotCancel()
    {
        std::string s;
        for (int i=0; i<2000; ++i)
            s+="v"+std::to_string(i)+"="+std::to_string(i)+";\n";
        iss_->setBackgroundParsingEnabled(false);
        script_=s;
        iss_->rebuild(IQISCADScriptModelGenerator::Rebuild);
        iss_->setBackgroundParsingEnabled(true); // triggers update

        script_="a=1;\n";
        iss_->requestUpdate();
        waitIdle(*iss_, 60000);

        QCOMPARE(rebuilt_, 1);
        QCOMPARE(*scalar("v1999"), 1999.);
        // background parse of the new script was done after the rebuild
        QVERIFY(bool(iss_->lastParseResult()));
        QCOMPARE(iss_->lastParseResult()->script, std::string("a=1;\n"));
    }
};




QTEST_MAIN(ISCADScriptJobScheduling)

#include "ISCADScriptJobScheduling.moc"
