#include "base/tools.h"
#include "base/remoteexecution.h"
#include "base/sshlinuxserver.h"
#include "base/translations.h"

#include "remoterun.h"
#include "analysisform.h"
#include "ui_analysisform.h"


#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include "boost/process.hpp"
#include "base/warningdispatcher.h"

using namespace std;
using namespace boost;
namespace bf=boost::filesystem;
namespace bp=boost::process;




void UndoSteps::addUndoStep(UndoFunction undoFunction, const std::string& label)
{
    undoSteps_.insert(undoSteps_.begin(), { undoFunction, label });
}




void UndoSteps::performUndo(std::exception_ptr undoReason, bool rethrow)
{
    // take the steps out first: each step is executed only once,
    // even if the rollback is triggered again (e.g. by error and cancellation)
    UndoStepList steps;
    steps.swap(undoSteps_);

    // attempt to roll back
    for (auto& undoStep: steps)
    {
      try
      {
        undoStep.undoFunction(); // call undo function
      }
      catch (std::exception& re)
      {
        insight::Warning(
              "during rollback in step %s:\n %s",
              undoStep.label.c_str(), re.what() );
      }
      catch (...)
      {
        insight::Warning(
              "during rollback in step %s: unknown error",
              undoStep.label.c_str() );
      }
    }

    // rethrow
    if (rethrow && undoReason)
        std::rethrow_exception(undoReason);
}







RemoteRun::Timing& RemoteRun::defaultTiming()
{
    static Timing timing;
    return timing;
}




RemoteRun::RemoteRun(AnalysisForm *af, bool resume)
  : WorkbenchAction(af),
    timing_( defaultTiming() ),
    resume_( resume ),
    remote_( af->remoteExecutionConfiguration() ),
    rec_( std::make_unique<insight::RemoteExecutionConfig>(
              af->remoteExecutionConfiguration()->exeConfig() ) ),
    killRequested_(false), disconnectRequested_(false),
    ended_(false), contactEstablished_(false),
    launchProgress_( af_->progressDisplayer_.forkNewAction(
          4,
          _("Launching remote analysis")) )
{
    // presumption: all signals have to be emitted from another thread!
    connect(this, &RemoteRun::finished,
            af_, &AnalysisForm::onResultReady,
            Qt::QueuedConnection);

    connect(this, &RemoteRun::failed,
            af_, &AnalysisForm::onAnalysisError,
            Qt::QueuedConnection);

    connect(this, &RemoteRun::cancelled,
            af_, &AnalysisForm::onAnalysisCancelled,
            Qt::QueuedConnection);

    connect(this, &RemoteRun::statusMessage,
            [this](const QString& msg) { af_->statusMessage(msg); }
            );
}




void RemoteRun::launch()
{

  portMappings_ = (*rec_).server()->makePortsAccessible(
      { (*rec_).port() },
      {}
  );

  af_->progressDisplayer_.reset();
  af_->ui->tabWidget->setCurrentWidget(af_->ui->runTab);

  // GUI state is read here (GUI thread), not in the IO threads
  downloadWhenFinished_ =
      af_->ui->cbDownloadWhenFinished->checkState()==Qt::Checked;
  inputParameters_ = af_->parameters().cloneAs<insight::ParameterSet>();
  inputParameters_->pack();

  ac_ = std::make_unique<insight::AnalyzeClient>(
      af_->psmodel_->getAnalysisName(),
      str(format("http://"+(*rec_).server()->IPaddress()+":%d")
          % portMappings_->localListenerPort((*rec_).port()) ),
      &af_->progressDisplayer_
  );
  ac_->setTimeout( timing_.requestTimeout );

  if (!resume_)
  {
      launchProgress_->message(_("Setting up the remote workspace..."));
      ac_->ioService().post( std::bind(&RemoteRun::setupRemoteEnvironment, this) );
  }
  else
  {
      ac_->ioService().post( std::bind(&RemoteRun::monitor, this) );
  }
}




void RemoteRun::setupRemoteEnvironment()
{
    try {
        checkIfCancelled();

        insight::dbg()<<"initialize remote location"<<std::endl;

        auto &remexeenv = (*rec_);
        remexeenv.initialize(true);

        insight::assertion(
            remexeenv.isActive(),
            _("Remote directory is invalid!") );

        addUndoStep(
            std::bind(&RemoteRun::undoSetupRemoteEnvironment, this),
            _("setting up the execution environment")
        );

        launchProgress_->stepTo(1);
        launchProgress_->message(_("Launching remote execution server..."));

        ac_->ioService().post( std::bind(&RemoteRun::uploadInputFile, this) );

    } catch (...) { onError(std::current_exception()); }
}



void RemoteRun::undoSetupRemoteEnvironment()
{
    insight::dbg()<<"cleanup remote location"<<std::endl;
    // undo: if needed: remove remote work dir again, shutdown remote machine
    try
    {
        cleanupRemoteLocation();
    }
    catch (std::exception& e)
    {
        insight::Warning(_("Could not clean remote location! Reason: %s"), e.what());
    }
}




void RemoteRun::uploadInputFile()
{
    try
    {
        checkIfCancelled();

        insight::dbg()<<"upload input file"<<std::endl;

        {
            auto rs = (*rec_).remoteOFStream(
                "param.ist", 0 );
            inputParameters_->saveToStream(
                *rs,
                insight::hierarchicalData::Element::OutputProperties());
            rs->close(); // throws, if the remote file could not be written
        }

        launchProgress_->stepTo(2);
        launchProgress_->message(_("Writing input file in execution directory...") );

        ac_->ioService().post( std::bind(&RemoteRun::launchRemoteExecutionServer, this) );

    } catch (...) { onError(std::current_exception()); }
}




void RemoteRun::launchRemoteExecutionServer()
{
    try
    {
        checkIfCancelled();

        insight::dbg()<<"launch execution server"<<std::endl;

        auto rd = (*rec_).remoteDir();

        analyzeProcess_ = (*rec_).server()->launchBackgroundProcess(
                    "analyze "
                    " --workdir=\""+insight::toUnixPath(rd)+"\""
                    " --server"
                    +str(format(
                    " --port %d"
                             ) % (*rec_).port() )+
                    " param.ist >\""+insight::toUnixPath(rd/"analyze.log")+"\" 2>&1 </dev/null"
                    // the exit code is a hint in the error message, if analyze ends unexpectedly
                    "; echo \"[analyze exited with code $?]\" >>\""+insight::toUnixPath(rd/"analyze.log")+"\""
                    );

        addUndoStep( std::bind(&RemoteRun::undoLaunchRemoteExecutionServer, this),
                    _("launching the remote execution server") );

        launchProgress_->stepTo(3);
        launchProgress_->message(_("Establishing contact to remote execution server...") );

        ac_->ioService().post( std::bind(&RemoteRun::waitForContact, this, timing_.contactAttempts) );

    } catch (...) { onError(std::current_exception()); }
}



void RemoteRun::undoLaunchRemoteExecutionServer()
{
    insight::dbg()<<"kill remote server"<<std::endl;
    try
    {
        // undo: kill remote server process
        analyzeProcess_->kill();
    }
    catch (std::exception& e)
    {
        insight::Warning(_("Could not clean remote location! Reason: %s"), e.what());
    }
}



void RemoteRun::waitForContact( int maxAttempts )
{
    auto scheduleNextAttempt = [this,maxAttempts]() {
        if (disconnectRequested_) return;

        if (remoteAnalyzeHasExited())
        {
            onErrorString(
                _("The remote analysis server has exited during its startup.")
                +remoteAnalyzeLog() );
            return;
        }

        // schedule next attempt, if some remain
        if (maxAttempts>0)
        {
            insight::dbg()<<"schedule next contact attempt"<<std::endl;
            ac_->ioService().schedule(
                    timing_.contactInterval,
                    std::bind( &RemoteRun::waitForContact, this,
                               maxAttempts-1 ) );
        }
        else
        {
            insight::dbg()<<"cancel after too many attempts"<<std::endl;
            // cancel otherwise
            ac_->ioService().post(
                        std::bind(
                        &RemoteRun::onErrorString, this,
                    std::string{_("Could not contact analysis server after launching it!")}
                        + remoteAnalyzeLog() ) );
        }

    };

    try {

        insight::dbg()<<"waiting for contact"<<std::endl;

        checkIfCancelled();

        ac_->queryStatus(
                [this,scheduleNextAttempt](insight::QueryStatusAction::Result r)
                {
                    if (r.success)
                    {
                        contactEstablished_ = true;
                        // execute callback on success

                        launchProgress_->stepTo(4);
                        launchProgress_->message(_("Monitoring analysis..."));

                        ac_->ioService().post( std::bind( &RemoteRun::monitor, this ));
                    }
                    else
                    {
                        scheduleNextAttempt();
                    }
                },

                scheduleNextAttempt
        );

    } catch (...) { onError(std::current_exception()); }
}





// void RemoteRun::launchAnalysis()
// {
//     try {

//         insight::dbg()<<"packing parameter set"<<std::endl;

//         checkIfCancelled();

//         insight::ParameterSet p = af_->parameters();
//         p.packExternalFiles(); // pack

//         insight::dbg()<<"launch remote analysis"<<p<<std::endl;

//         ac_->launchAnalysis(
//                     p, "/", af_->analysisName_,

//                     // on response
//                     [&](insight::AnalyzeClientAction::ReportSuccessResult r)
//                     {
//                         if (r.success)
//                         {
//                             launchProgress_->stepTo(5);
//                             launchProgress_->message(_("Monitoring remote run"));
//                             launchProgress_->completed();

//                             ac_->ioService().post(std::bind(
//                                                       &RemoteRun::monitor, this));
//                         }
//                         else
//                         {
//                             ac_->ioService().post(std::bind(
//                                                       &RemoteRun::onErrorString, this,
//                                 _("Failed to start analysis on remote server!")));
//                         }
//                     },

//                     // on timeout
//                     [&]()
//                     {
//                         ac_->ioService().post(std::bind(
//                                                   &RemoteRun::onErrorString, this,
//                     _("No response after starting analysis on remote server!")));
//                     }
//         );

//     } catch(...) { onError(std::current_exception()); }
// }




void RemoteRun::monitor()
{
    insight::dbg()<<"query status"<<std::endl;

    try {

        checkIfCancelled();

        if (disconnectRequested_) return;

        ac_->queryStatus(
                    [this](insight::QueryStatusAction::Result qsr)
                    {
                      try
                      {
                        if (disconnectRequested_) return;

                        if (!qsr.success)
                        {
                            // no answer: the server may have died or the connection be broken
                            ++failedStatusQueries_;
                            if (remoteAnalyzeHasExited())
                            {
                                onErrorString(
                                    _("The remote analysis server has exited unexpectedly.")
                                    +remoteAnalyzeLog() );
                            }
                            else if (failedStatusQueries_ >= timing_.maxFailedStatusQueries)
                            {
                                onErrorString(
                                    str(format(_("Lost contact to the remote analysis server (%d failed status queries)."))
                                        % failedStatusQueries_)
                                    +remoteAnalyzeLog() );
                            }
                            else
                            {
                                ac_->ioService().schedule(
                                            timing_.pollInterval,
                                            std::bind(&RemoteRun::monitor, this) );
                            }
                            return;
                        }
                        failedStatusQueries_ = 0;
                        contactEstablished_ = true;

                        if (qsr.errorOccurred)
                        {
                            onError(std::make_exception_ptr(*qsr.exception));
                        }
                        else
                        {
                            if (qsr.resultsAreAvailable)
                            {
                                // proceed with result query
                                ac_->ioService().post( std::bind(&RemoteRun::fetchResults, this) );
                            }
                            else
                            {
                                // schedule next status query
                                ac_->ioService().schedule(
                                            timing_.pollInterval,
                                            std::bind(&RemoteRun::monitor, this) );
                            }
                        }
                      }
                      catch (...) { onError(std::current_exception()); }
                    },

                    std::bind( &RemoteRun::onErrorString, this,
                      _("timeout in quering status of analysis server") )
        );

    } catch(...) { onError(std::current_exception()); }
}




void RemoteRun::fetchResults()
{
    insight::dbg()<<"query results"<<std::endl;
    try {

        checkIfCancelled();

        if (disconnectRequested_) return;

        ac_->queryResults(
                    [this](insight::QueryResultsAction::Result qrs)
                    {
                        results_=std::move(qrs.results);

                        ac_->ioService().post(
                                    std::bind(&RemoteRun::stopRemoteExecutionServer, this ) );
                    },

            std::bind(&RemoteRun::onErrorString, this, _("timeout while fetching results"))
        );

    } catch(...) { onError(std::current_exception()); }
}




void RemoteRun::stopRemoteExecutionServer()
{
    insight::dbg()<<"exit server"<<std::endl;
    try {

        checkIfCancelled();

        ac_->exit(
                    [this](insight::AnalyzeClientAction::ReportSuccessResult rs)
                    {
                insight::dbg() << _("stop server")<<" "
                               << (rs.success?_("was"):_("was not"))
                               << " "<<_("successful")<<" " <<std::endl;

                        if (disconnectRequested_) return;
                        ac_->ioService().post(
                                    downloadWhenFinished_ ?
                                    std::bind(&RemoteRun::download, this) :
                                    std::bind(&RemoteRun::cleanupRemote, this)
                                    );
                    },

                    std::bind(&RemoteRun::onErrorString, this,
                      _("timeout while stopping remote analysis server"))
                );

    } catch(...) { onError(std::current_exception()); }
}




void RemoteRun::download()
{
    try
    {
        checkIfCancelled();
        if (disconnectRequested_) return;

        auto progress = af_->progressDisplayer_.forkNewAction(
            100, _("Downloading results") );

        rec_->syncToLocal(
            false, false, {},
            [progress](int p, const std::string& msg)
            {
                progress->stepTo(p);
                progress->message(msg);
            } );

        progress->completed();

        ac_->ioService().post(
                std::bind(&RemoteRun::cleanupRemote, this) );

    } catch (...) { onError(std::current_exception()); }
}




void RemoteRun::cleanupRemote()
{
    try
    {
        checkIfCancelled();

        cleanupRemoteLocation();

        finish();

    } catch (...) { onError(std::current_exception()); }
}




void RemoteRun::cleanupRemoteLocation()
{
    if (rec_->isTemporaryStorage())
    {
        insight::dbg()<<"cleanup"<<std::endl;
        rec_->cleanup(); // remote work, no GUI access

        // the remote location does not exist anymore: remove it from the GUI
        QPointer<IQRemoteExecutionState> remote = remote_;
        if (remote)
        {
            QMetaObject::invokeMethod(
                remote.data(),
                [remote]() { if (remote) remote->discard(); },
                Qt::QueuedConnection );
        }
    }
}




void RemoteRun::finish()
{
    if (!claimEnd()) return;
    insight::dbg()<<"emit finished"<<std::endl;
    Q_EMIT finished();
}




bool RemoteRun::claimEnd()
{
    return !ended_.exchange(true);
}




void RemoteRun::checkIfCancelled()
{
    if (killRequested_)
    {
        throw insight::Exception(_("remote run cancelled"));
    }
}




void RemoteRun::onErrorString(const std::string& errorMessage)
{
    // the message must not be interpreted as format string (e.g. log output)
    onError( std::make_exception_ptr(
                 insight::Exception("%s", errorMessage.c_str()) ) );
}




std::string RemoteRun::remoteAnalyzeLog()
{
    std::string logFile;
    try
    {
        auto& rec = (*rec_);
        logFile = insight::toUnixPath(rec.remoteDir()/"analyze.log");
        auto lf = insight::shellQuote(logFile);

        // exit code 3: no log file
        auto r = rec.server()->config().runCommand(
            "test -e "+lf+" || exit 3; tail -n 30 "+lf );

        if (r.exitCode==3)
        {
            return "\n\n"+str(format(_(
                "The log file %s of the remote analysis server does not exist:"
                " the server process was not started"
                " (or it was terminated before it could run).")) % logFile);
        }
        else if (r.exitCode!=0)
        {
            return "\n\n"+str(format(_(
                "The log file %s of the remote analysis server could not be read"
                " (exit code %d): %s")) % logFile % r.exitCode % r.err);
        }
        else if (r.out.empty())
        {
            return "\n\n"+str(format(_(
                "The log file %s of the remote analysis server is empty.")) % logFile);
        }

        return "\n\n"+str(format(_("Last output of the remote analysis server (%s):")) % logFile)
               +"\n"+r.out;
    }
    catch (const std::exception& e)
    {
        return "\n\n"+str(format(_(
            "The log file %s of the remote analysis server could not be read: %s"))
                % logFile % e.what());
    }
}




bool RemoteRun::remoteAnalyzeHasExited()
{
    try
    {
        return analyzeProcess_ && !analyzeProcess_->isRunning();
    }
    catch (...)
    {
        return false; // unknown
    }
}



void RemoteRun::onError(std::exception_ptr ex)
{
    // after a cancellation request, the cancellation path does the rollback
    // (errors are then mostly consequences of the cancellation)
    if (killRequested_ || !claimEnd()) return;

    performUndo(ex, false);
    Q_EMIT failed(ex);
}




RemoteRun* RemoteRun::create(AnalysisForm* af, bool resume)
{
    auto *a = new RemoteRun(af, resume);
    a->launch();
    return a;
}


RemoteRun::~RemoteRun()
{
    disconnectRequested_=true;
    // stop the client, while it is still accessible:
    // handlers, which are currently running, may still use it
    if (ac_)
    {
        ac_->shutdown();
    }
}



std::unique_ptr<insight::ResultSet> RemoteRun::moveResults()
{
    return std::move(results_);
}




void RemoteRun::onCancel()
{
    if (killRequested_.exchange(true)) return; // already cancelling

    launchProgress_->message(_("Stopping remote analysis..."));
    // don't wait for a pending (status) request
    ac_->httpClient().abort();
    ac_->ioService().post( std::bind(&RemoteRun::cancelRemoteRun, this) );
}




void RemoteRun::cancelRemoteRun()
{
    if (!claimEnd()) return; // has ended already

    insight::dbg()<<"cancel remote run"<<std::endl;

    if (!contactEstablished_ && !resume_)
    {
        // the remote server was not yet contacted:
        // the rollback stops it, if it was launched
        completeCancellation(false);
        return;
    }

    // request interruption of the analysis and termination of the server.
    // This works also for resumed runs, which did not launch the server process.
    auto requestExit = [this](bool killSucceeded)
    {
        ac_->exit(
            [this,killSucceeded](insight::AnalyzeClientAction::ReportSuccessResult r)
            { completeCancellation(killSucceeded || r.success); },
            [this,killSucceeded]()
            { completeCancellation(killSucceeded); },
            timing_.cancelRequestTimeout );
    };

    ac_->kill(
        [requestExit](insight::AnalyzeClientAction::ReportSuccessResult r)
        { requestExit(r.success); },
        [requestExit]()
        { requestExit(false); },
        timing_.cancelRequestTimeout );
}




void RemoteRun::completeCancellation(bool remoteServerStopped)
{
    insight::dbg()<<"complete cancellation"<<std::endl;

    // kills the launched server process and removes the remote location,
    // if they were created by this run
    performUndo(nullptr, false);

    if (resume_)
    {
        if (remoteServerStopped)
        {
            try
            {
                cleanupRemoteLocation();
            }
            catch (std::exception& e)
            {
                insight::Warning(_("Could not clean remote location! Reason: %s"), e.what());
            }
        }
        else
        {
            insight::Warning(
                _("The remote analysis server could not be stopped."
                  " The remote location is kept."));
        }
    }

    Q_EMIT cancelled();
}


