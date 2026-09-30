/*
 * RemoteRun (src/workbench/remoterun.cpp) driven through an offscreen AnalysisForm
 * with the "Dummy Analysis".
 *
 * After each case it is checked, that no remote analyze process is left running
 * and that the temporary remote directory has been removed.
 */

#include <QApplication>
#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

#include "analysisform.h"
#include "remoterun.h"
#include "qactionprogressdisplayerwidget.h"

#include "base/parameters/subsetparameter.h"
#include "base/remotelocation.h"
#include "base/remotecommand.h"

#include "backends.h"
#include "fakeanalyzeserver.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;
namespace fs = boost::filesystem;




/**
 * catches exceptions, which escape from slots
 * (AnalysisForm::onAnalysisError rethrows the error of the run)
 */
class TestApplication : public QApplication
{
public:
    std::vector<std::string> exceptions;

    TestApplication(int& argc, char** argv)
        : QApplication(argc, argv)
    {}

    bool notify(QObject* receiver, QEvent* e) override
    {
        try
        {
            return QApplication::notify(receiver, e);
        }
        catch (const std::exception& ex)
        {
            exceptions.push_back(ex.what());
        }
        catch (...)
        {
            exceptions.push_back("(unknown exception)");
        }
        return false;
    }
};




/**
 * closes modal message boxes (e.g. "The analysis has finished")
 * and records their titles
 */
class ModalDialogCloser : public QObject
{
public:
    std::vector<QString> titles;
    QTimer timer;

    ModalDialogCloser()
    {
        connect(&timer, &QTimer::timeout, this, [this]()
        {
            if (auto *w = QApplication::activeModalWidget())
            {
                titles.push_back(w->windowTitle());
                if (auto *mb = qobject_cast<QMessageBox*>(w))
                {
                    titles.back() += ": "+mb->text();
                    mb->done(QMessageBox::Ok);
                }
                else
                    w->close();
            }
        });
        timer.start(100);
    }

    bool sawTitle(const QString& t) const
    {
        for (const auto& s: titles)
            if (s.startsWith(t)) return true;
        return false;
    }
};




class TestAnalysisForm : public AnalysisForm
{
public:
    using AnalysisForm::AnalysisForm;

    WorkbenchAction* currentAction()
    {
        return currentWorkbenchAction_.data();
    }

    void setDummyParameters(double executionTime, int deltaT, bool emitError)
    {
        auto ps = parameters().cloneAs<ParameterSet>();
        ps->setDouble("executionTime", executionTime);
        ps->setInt("deltaT", deltaT);
        ps->setBool("emitError", emitError);
        psmodel_->resetParameterValues(*ps);
    }

    template<class W>
    W* widget(const char* name)
    {
        auto *w = findChild<W*>(name);
        check(w!=nullptr, std::string("widget not found: ")+name);
        return w;
    }
};




void processEventsFor(std::chrono::milliseconds t)
{
    auto deadline=std::chrono::steady_clock::now()+t;
    while (std::chrono::steady_clock::now()<deadline)
    {
        QApplication::processEvents(QEventLoop::AllEvents, 50);
        std::this_thread::sleep_for(20ms);
    }
}


bool processEventsUntil(std::function<bool()> cond, std::chrono::milliseconds timeout)
{
    auto deadline=std::chrono::steady_clock::now()+timeout;
    while (std::chrono::steady_clock::now()<deadline)
    {
        QApplication::processEvents(QEventLoop::AllEvents, 50);
        if (cond()) return true;
        std::this_thread::sleep_for(20ms);
    }
    return cond();
}




/**
 * one analysis form with a remote execution configuration
 */
struct RemoteRunFixture
{
    TestApplication& app;
    RemoteBackend& be;
    ModalDialogCloser closer;
    QWidget host;
    std::unique_ptr<insight::IQActionProgressDisplayManager> apm;
    TestAnalysisForm* form;
    TemporaryDirectory localDir;
    std::unique_ptr<RemoteLocation> remoteLocation;

    // captured at launch
    int port=-1;
    fs::path remoteDir;

    size_t nExceptionsBefore;

    RemoteRunFixture(TestApplication& a, RemoteBackend& b, bool persistentLocalDir)
        : app(a), be(b)
    {
        nExceptionsBefore = app.exceptions.size();
        apm = std::make_unique<insight::IQActionProgressDisplayManager>(&host);
        form = new TestAnalysisForm(&host, apm.get(), "Dummy Analysis", true);
        form->setAttribute(Qt::WA_DeleteOnClose, false);

        remoteLocation = std::make_unique<RemoteLocation>(be.serverConfig());
        form->resetExecutionEnvironment(
            persistentLocalDir ? localDir.path() : fs::path(),
            remoteLocation.get() );
        check(form->remoteExecutionConfiguration()!=nullptr, "remote configuration not set");
    }

    ~RemoteRunFixture()
    {
        // stop anything, which might still be running
        try
        {
            if (port>0)
            {
                auto pat = processPattern();
                if (!be.remoteProcessesMatching(pat).empty())
                    be.remoteOutput("pkill -f -- '"+pat+"'");
            }
            if (!remoteDir.empty() && be.remoteDirectoryExists(remoteDir))
                be.server()->removeDirectory(remoteDir);
        }
        catch (...) {}
        delete form;
        processEventsFor(200ms);
    }

    std::string processPattern() const
    {
        return "analyze.*--port "+std::to_string(port);
    }

    void setDownload(bool download)
    {
        form->widget<QCheckBox>("cbDownloadWhenFinished")->setChecked(download);
    }

    void start()
    {
        form->widget<QPushButton>("btnRun")->click();
        check(form->currentAction()!=nullptr, "no action started");
        check(dynamic_cast<RemoteRun*>(form->currentAction())!=nullptr, "started action is not a remote run");

        auto& rec = form->remoteExecutionConfiguration()->exeConfig();
        port = rec.port();
        remoteDir = rec.remoteDir();
    }

    bool running()
    {
        return form->currentAction()!=nullptr;
    }

    std::vector<std::string> newExceptions() const
    {
        return std::vector<std::string>(
            app.exceptions.begin()+nExceptionsBefore, app.exceptions.end() );
    }

    bool waitUntilRemoteAnalyzeRuns(std::chrono::milliseconds t)
    {
        return processEventsUntil(
            [this]{ return !be.remoteProcessesMatching(processPattern()).empty(); }, t );
    }

    void checkRemoteCleanedUp(std::chrono::milliseconds t = 15s)
    {
        bool gone = processEventsUntil(
            [this]{ return be.remoteProcessesMatching(processPattern()).empty(); }, t );
        check(gone, "remote analyze process (port %d) is still running", port);

        bool removed = processEventsUntil(
            [this]{ return !be.remoteDirectoryExists(remoteDir); }, t );
        check(removed, "temporary remote directory %s still exists", remoteDir.c_str());
    }
};




int main(int argc, char* argv[])
{
    TestApplication app(argc, argv);

    TestRunner tr("remote_workbench_remoterun", argc, argv);

    auto be = availableBackendFromCommandLine(argc, argv);
    if (!be) return SKIP_RETURN_CODE;

    // short intervals to keep the test fast
    auto& timing = RemoteRun::defaultTiming();
    timing.contactAttempts = 20;
    timing.contactInterval = 500ms;
    timing.pollInterval = 200ms;
    timing.requestTimeout = 5s;

    {
        BackendSession session(be);


        tr.run("successful remote run", [&]()
        {
            RemoteRunFixture f(app, *be, false);
            f.form->setDummyParameters(3, 100, false);
            f.setDownload(false);
            f.start();

            bool done = processEventsUntil([&]{ return !f.running(); }, 180s);
            check(done, "remote run did not finish within 180 s");
            check(f.closer.sawTitle("Finished!"), "run did not finish successfully");
            check(f.newExceptions().empty(), "unexpected error: "+
                  (f.newExceptions().empty() ? std::string() : f.newExceptions().front()));

            f.checkRemoteCleanedUp();
        });


        tr.run("successful remote run with download", [&]()
        {
            RemoteRunFixture f(app, *be, true);
            f.form->setDummyParameters(3, 100, false);
            f.setDownload(true);
            f.start();

            bool done = processEventsUntil([&]{ return !f.running(); }, 180s);
            check(done, "remote run did not finish within 180 s");
            check(f.closer.sawTitle("Finished!"), "run did not finish successfully");
            check(fs::exists(f.localDir.path()/"analyze.log"),
                  "remote files were not downloaded");

            f.checkRemoteCleanedUp();
        });


        tr.run("error in remote analysis is reported", [&]()
        {
            RemoteRunFixture f(app, *be, false);
            f.form->setDummyParameters(2, 100, true);
            f.setDownload(false);
            f.start();

            bool done = processEventsUntil([&]{ return !f.running(); }, 180s);
            check(done, "remote run did not end within 180 s");
            auto ex = f.newExceptions();
            check(ex.size()==1, "expected exactly one reported error, got %d", int(ex.size()));
            check(ex[0].find("You wanted this error.")!=std::string::npos,
                  "unexpected error: "+ex[0]);

            f.checkRemoteCleanedUp();
        });


        tr.run("cancellation stops the remote analysis", [&]()
        {
            RemoteRunFixture f(app, *be, false);
            f.form->setDummyParameters(600, 200, false);
            f.setDownload(false);
            f.start();

            check(f.waitUntilRemoteAnalyzeRuns(60s), "remote analyze did not start");
            processEventsFor(2s);

            f.form->widget<QPushButton>("btnKill")->click();

            bool done = processEventsUntil([&]{ return !f.running(); }, 30s);
            check(done, "run did not end after cancellation");
            check(f.closer.sawTitle("Stopped!"), "cancellation was not reported");

            f.checkRemoteCleanedUp();
        });


        tr.run("dying remote analyze is detected", [&]()
        {
            RemoteRunFixture f(app, *be, false);
            f.form->setDummyParameters(600, 200, false);
            f.setDownload(false);
            f.start();

            check(f.waitUntilRemoteAnalyzeRuns(60s), "remote analyze did not start");
            processEventsFor(3s); // let the monitoring start

            be->remoteOutput("pkill -9 -f -- '"+f.processPattern()+"'");

            bool done = processEventsUntil([&]{ return !f.running(); }, 60s);
            check(done, "loss of the remote analyze process was not detected within 60 s");
            check(!f.newExceptions().empty(), "loss of the remote process should be reported as error");
        });


        tr.run("failing launch of the remote server is reported", [&]()
        {
            if (!be->isLocalMachine())
                throw SkipCase("port occupation requires a backend on the local machine");

            RemoteRunFixture f(app, *be, false);
            f.form->setDummyParameters(600, 200, false);
            f.setDownload(false);

            // occupy the port, which will be used by the remote analyze server
            std::unique_ptr<PortBlocker> blocker;
            f.form->widget<QPushButton>("btnRun")->click();
            check(f.running(), "no action started");
            auto& rec = f.form->remoteExecutionConfiguration()->exeConfig();
            f.port = rec.port();
            f.remoteDir = rec.remoteDir();
            try { blocker = std::make_unique<PortBlocker>(f.port, true); }
            catch (...) { throw SkipCase("port was already taken by the analyze server"); }

            bool done = processEventsUntil([&]{ return !f.running(); }, 120s);
            check(done, "failing server launch was not detected within 120 s");
            auto ex = f.newExceptions();
            check(!ex.empty(), "failure should be reported as error");
            check(ex.back().find("web server")!=std::string::npos
                  || ex.back().find("analyze.log")!=std::string::npos,
                  "error should contain the reason from the remote log: "+ex.back());

            f.checkRemoteCleanedUp();
        });


        tr.run("failing download is reported", [&]()
        {
            RemoteRunFixture f(app, *be, true);
            f.form->setDummyParameters(1, 100, false);
            f.setDownload(true);
            f.start();

            // make the local directory unwritable
            fs::permissions(f.localDir.path(), fs::owner_read|fs::owner_exe);

            bool done = processEventsUntil([&]{ return !f.running(); }, 180s);
            fs::permissions(f.localDir.path(), fs::owner_all);

            check(done, "run with failing download did not end within 180 s");
            check(!f.closer.sawTitle("Finished!"), "run with failing download reported success");
            check(!f.newExceptions().empty(), "failing download should be reported as error");
        });


        tr.run("disconnect and resume", [&]()
        {
            RemoteRunFixture f(app, *be, false);
            f.form->setDummyParameters(8, 100, false);
            f.setDownload(false);
            f.start();

            check(f.waitUntilRemoteAnalyzeRuns(60s), "remote analyze did not start");
            processEventsFor(2s);

            auto *btnDisconnect = f.form->widget<QPushButton>("btnDisconnect");
            if (!btnDisconnect->isEnabled())
                throw SkipCase("disconnect is not supported for this backend");
            btnDisconnect->click();
            check(!f.running(), "still running after disconnect");
            check(!be->remoteProcessesMatching(f.processPattern()).empty(),
                  "remote analyze should continue after disconnect");

            f.form->widget<QPushButton>("btnResume")->click();
            check(f.running(), "resume did not start an action");

            bool done = processEventsUntil([&]{ return !f.running(); }, 180s);
            check(done, "resumed run did not finish within 180 s");
            check(f.closer.sawTitle("Finished!"), "resumed run did not finish successfully");

            f.checkRemoteCleanedUp();
        });


        tr.run("cancellation after resume stops the remote analysis", [&]()
        {
            RemoteRunFixture f(app, *be, false);
            f.form->setDummyParameters(600, 200, false);
            f.setDownload(false);
            f.start();

            check(f.waitUntilRemoteAnalyzeRuns(60s), "remote analyze did not start");
            processEventsFor(2s);

            auto *btnDisconnect = f.form->widget<QPushButton>("btnDisconnect");
            if (!btnDisconnect->isEnabled())
                throw SkipCase("disconnect is not supported for this backend");
            btnDisconnect->click();
            f.form->widget<QPushButton>("btnResume")->click();
            processEventsFor(2s);

            f.form->widget<QPushButton>("btnKill")->click();
            bool done = processEventsUntil([&]{ return !f.running(); }, 30s);
            check(done, "run did not end after cancellation");

            f.checkRemoteCleanedUp();
        });
    }

    return tr.finish();
}
