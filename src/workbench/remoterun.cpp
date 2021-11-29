#include "base/tools.h"
#include "base/remoteexecution.h"
#include "base/sshlinuxserver.h"

#include "remoterun.h"
#include "analysisform.h"
#include "ui_analysisform.h"


#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include "boost/process.hpp"

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
    // attempt to roll back
    for (auto& undoStep: undoSteps_)
    {
      try
      {
        undoStep.undoFunction(); // call undo function
      }
      catch (std::exception& re)
      {

        insight::Warning(
              boost::str(boost::format("during rollback in step %s:\n %s")
                         % undoStep.label % re.what())
              );
      }
    }

    // rethrow
    if (rethrow) throw undoReason;
}







RemoteRun::RemoteRun(AnalysisForm *af, insight::RemoteExecutionConfig& rec, bool resume)
  : WorkbenchAction(af),
    resume_( resume ),
    remote_( rec ),
    cancelled_(false)
{}




void RemoteRun::launch()
{

  portMappings_ = remote_.server()->makePortsAccessible(
      {8090},
      {}
  );

  af_->progressDisplayer_.reset();
  af_->ui->tabWidget->setCurrentWidget(af_->ui->runTab);

  ac_ = std::make_unique<insight::AnalyzeClient>(
      af_->analysisName_,
      str(format("http://127.0.0.1:%d") % portMappings_->localListenerPort(8090) ),
      &af_->progressDisplayer_
  );

  if (!resume_)
      ac_->ioService().post( std::bind(&RemoteRun::setupRemoteEnvironment, this) );
  else
      ac_->ioService().post( std::bind(&RemoteRun::monitor, this) );
}




void RemoteRun::setupRemoteEnvironment()
{
    try {
        checkIfCancelled();

        insight::dbg()<<"initialize remote location"<<std::endl;

        remote_.initialize();

        if (!remote_.isActive())
            throw insight::Exception("Remote directory is invalid!");

        addUndoStep( std::bind(&RemoteRun::undoSetupRemoteEnvironment, this),
                     "setting up the execution environment" );
        ac_->ioService().post( std::bind(&RemoteRun::launchRemoteExecutionServer, this) );

    } catch (...) { onError(std::current_exception()); }
}



void RemoteRun::undoSetupRemoteEnvironment()
{
    insight::dbg()<<"cleanup remote location"<<std::endl;
    // undo: if needed: remove remote work dir again, shutdown remote machine
    try
    {
        remote_.cleanup();
    }
    catch (std::exception& e)
    {
        insight::Warning(std::string("Could not clean remote location! Reason: ")+e.what());
    }
}




void RemoteRun::launchRemoteExecutionServer()
{
    try
    {
        checkIfCancelled();

        insight::dbg()<<"launch execution server"<<std::endl;

        auto rd = remote_.remoteDir();

        analyzeProcess_ = remote_.server()->launchBackgroundProcess(
                    "analyze "
                    " --savecfg param.ist"
                    " --workdir=\""+insight::toUnixPath(rd)+"\""
                    " --server"
                    " >\""+insight::toUnixPath(rd/"analyze.log")+"\" 2>&1 </dev/null"
                    );

        addUndoStep( std::bind(&RemoteRun::undoLaunchRemoteExecutionServer, this),
                     "launching the remote execution server" );
        ac_->ioService().post( std::bind(&RemoteRun::waitForContact, this, 20) );

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
        insight::Warning(std::string("Could not clean remote location! Reason: ")+e.what());
    }
}



void RemoteRun::waitForContact( int maxAttempts )
{
    auto scheduleNextAttempt = [this,maxAttempts]() {
        // schedule next attempt in 10 secs, if some remain
        if (maxAttempts>0)
        {
            ac_->ioService().schedule(
                    std::chrono::seconds(2),
                    std::bind( &RemoteRun::waitForContact, this,
                               maxAttempts-1 ) );
        }
        else
        {
            // cancel otherwise
            ac_->ioService().post(
                        std::bind(
                        &RemoteRun::onErrorString, this,
                            std::string{"Could not contact analysis server after launching it!"} ) );
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
                        // execute callback on success
                        ac_->ioService().post( std::bind( &RemoteRun::launchAnalysis, this ));
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





void RemoteRun::launchAnalysis()
{
    try {

        insight::dbg()<<"packing parameter set"<<std::endl;

        checkIfCancelled();

        insight::ParameterSet p = af_->parameters();
        p.packExternalFiles(); // pack

        insight::dbg()<<"launch remote analysis"<<p<<std::endl;

        ac_->launchAnalysis(
                    p, "/", af_->analysisName_,

                    // on response
                    [&](insight::AnalyzeClientAction::ReportSuccessResult r)
                    {
                        if (r.success)
                        {
                            ac_->ioService().post(std::bind(
                                                      &RemoteRun::monitor, this));
                        }
                        else
                        {
                            ac_->ioService().post(std::bind(
                                                      &RemoteRun::onErrorString, this,
                                                      "Failed to start analysis on remote server!"));
                        }
                    },

                    // on timeout
                    [&]()
                    {
                        ac_->ioService().post(std::bind(
                                                  &RemoteRun::onErrorString, this,
                                                  "No response after starting analysis on remote server!"));
                    }
        );

    } catch(...) { onError(std::current_exception()); }
}




void RemoteRun::monitor()
{
    insight::dbg()<<"query status"<<std::endl;

    try {

        checkIfCancelled();

        ac_->queryStatus(
                    [this](insight::QueryStatusAction::Result qsr)
                    {
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
                                            std::chrono::milliseconds(1000),
                                            std::bind(&RemoteRun::monitor, this) );
                            }
                        }
                    },

                    std::bind( &RemoteRun::onErrorString, this,
                               "timeout in quering status of analysis server" )
        );

    } catch(...) { onError(std::current_exception()); }
}




void RemoteRun::fetchResults()
{
    insight::dbg()<<"query results"<<std::endl;
    try {

        checkIfCancelled();

        ac_->queryResults(
                    [this](insight::QueryResultsAction::Result qrs)
                    {
                        insight::dbg()<<"emit finished"<<std::endl;
                        Q_EMIT finished( qrs.results );

                        ac_->ioService().post(
                                    std::bind(&RemoteRun::stopRemoteExecutionServer, this ) );
                    },

                    std::bind(&RemoteRun::onErrorString, this, "timeout while fetching results")
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
                        insight::dbg() << "stop server "
                                       << (rs.success?"was":"was not")
                                       << " successful " <<std::endl;

                        ac_->ioService().post( std::bind(&RemoteRun::cleanupRemote, this) );
                    },

                    std::bind(&RemoteRun::onErrorString, this,
                              "timeout while stopping remote analysis server")
                );

    } catch(...) { onError(std::current_exception()); }
}




void RemoteRun::cleanupRemote()
{
    try
    {

        checkIfCancelled();

        insight::dbg()<<"cleanup"<<std::endl;
        remote_.cleanup();

    } catch (...) { onError(std::current_exception()); }
}




void RemoteRun::checkIfCancelled()
{
    if (cancelled_)
    {
        throw insight::Exception("remote run cancelled");
    }
}




void RemoteRun::onErrorString(const std::string& errorMessage)
{
    onError( std::make_exception_ptr(
                 insight::Exception(errorMessage) ) );
}



void RemoteRun::onError(std::exception_ptr ex)
{
    performUndo(ex, false);
    Q_EMIT failed(ex);
}




std::unique_ptr<RemoteRun> RemoteRun::create(AnalysisForm* af, insight::RemoteExecutionConfig& rec, bool resume)
{
    std::unique_ptr<RemoteRun> a(new RemoteRun(af, rec, resume));
    a->launch();
    return a;
}


RemoteRun::~RemoteRun()
{
    af_->progressDisplayer_.reset();
}




void RemoteRun::onCancel()
{
    ac_->httpClient().abort();
    cancelled_=true;
}


