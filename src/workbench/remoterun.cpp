#include "base/tools.h"
#include "base/remoteexecution.h"

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



ExecutionProgram::ExecutionProgram(const StepList &steps)
  : steps_(steps)
{}


void ExecutionProgram::run()
{
  for (int step=0; step < steps_.size(); ++step)
  {
    try
    {
      steps_[step].first(); // call run function
    }
    catch (std::exception& e)
    {
      // attempt to roll back
      for (int backStep=step; backStep>=0; --backStep)
      {
        try
        {
          steps_[backStep].second(); // call undo function
        }
        catch (std::exception& re)
        {
          insight::Warning(std::string("Exception occurred during rollback: ")+re.what());
        }
      }

      // rethrow
      throw e;
    }
  }
}


void RemoteRun::launchRemoteAnalysisServer()
{
  auto rd = remote_.remoteDir();
  int ret = remote_.execRemoteCmd(
        "analyze "
        "--workdir=\""+rd.string()+"\" "
        "--server" //>\""+rd.string()+"/server.log\" 2>&1 </dev/null &"
      );

  if (ret!=0)
    throw insight::Exception("Failed to launch remote analysis server executable!");
}







RemoteRun::RemoteRun(AnalysisForm *af, const insight::RemoteExecutionConfig& rec, bool resume)
  : WorkbenchAction(af),
    resume_( resume ),
    remote_( rec )
{}

void RemoteRun::launch()
{

  portMappings_ = remote_.server()->makePortsAccessible(
      {8090},
      {}
  );

  af_->progressDisplayer_.reset();
  af_->ui->tabWidget->setCurrentWidget(af_->ui->runTab);

  ac_.reset(
        new insight::AnalyzeClient(
          af_->analysisName_,
          str(format("http://127.0.0.1:%d") % portMappings_->localListenerPort(8090) ),
          &af_->progressDisplayer_,
          [this](std::exception_ptr e)
            {
              workerThread_->interrupt();
              Q_EMIT failed(e);
            }
       )
  );


  ExecutionProgram::StepList initSteps =
  {

    // check remote work dir, launch machine and create, if needed
    {
      [&]()
      {
        insight::dbg()<<"initialize remote location"<<std::endl;
        remote_.initialize();
      },
      [&]()
      {
        insight::dbg()<<"cleanup remote location"<<std::endl;
        // undo: if needed: remove remote work dir again, shutdown remote machine
        remote_.cleanup();
      }
    },

    // launch remote execution server
    {
      [&]()
      {
        insight::dbg()<<"launch execution server"<<std::endl;

        auto rd = remote_.remoteDir();

        boost::process::ipstream is;
        analyzeProcess_ = remote_.server()->launchBackgroundProcess(
              "analyze "
              "--workdir=\""+rd.string()+"\" "
              "--server"
            );
      },
      [&]() {} // nothing to undo
    },

    // contact remote exec server
    {
      [&]()
      {
        insight::dbg()<<"waiting for contact"<<std::endl;

        if (!ac_->waitForContact())
          throw insight::Exception("Could not contact analysis server after launching it!");
      },
      [&]()
      {
        insight::dbg()<<"kill remote server"<<std::endl;

        // undo: kill remote server process
        analyzeProcess_->kill();
      }
    },

    {
      [&]()
      {
        insight::dbg()<<"packing parameter set"<<std::endl;

        insight::ParameterSet p = af_->parameters();
        p.packExternalFiles(); // pack

        insight::dbg()<<"launch remote analysis"<<std::endl;

        auto res = ac_->launchAnalysisSync(
            p, "/", af_->analysisName_ );

        if (!res.success)
          throw insight::Exception("Failed to start analysis on remote server!");
      },
      [&]() {} // no undo
    }

  };




  // start
  workerThread_.reset( new insight::QAnalysisThread(
                         [this,initSteps]
  {

    // launch everything, if we do not resume
    if (!resume_)
      ExecutionProgram(initSteps).run();

    // continue with monitoring
    for ( bool runMonitor=true; runMonitor; )
    {
      if (!ac_->isBusy())
      {
        insight::dbg()<<"query status"<<std::endl;

        auto qsr = ac_->queryStatusSync();

        if (qsr.resultsAreAvailable)
        {
          runMonitor=false;

          auto qrs = ac_->queryResultsSync();

          Q_EMIT finished( qrs.results );

          ac_->exitSync();
          remote_.cleanup();
        }

        boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
      }
    };

  }
  ) );

  connectAnalysisThread(workerThread_.get());
}

std::unique_ptr<RemoteRun> RemoteRun::create(AnalysisForm* af, const insight::RemoteExecutionConfig& rec, bool resume)
{
  std::unique_ptr<RemoteRun> a(new RemoteRun(af, rec, resume));
  a->launch();
  return a;
}


RemoteRun::~RemoteRun()
{
  workerThread_->join();
  if (cancelThread_.joinable()) cancelThread_.join();
}




void RemoteRun::onCancel()
{
  workerThread_->interrupt();

  cancelThread_ = boost::thread(
        [&]()
        {
            // wait for workerThread to end
            while (!workerThread_->try_join_for(boost::chrono::milliseconds(1)))
            {
              boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
            }

            // stop and exit
            ac_->killSync();

            Q_EMIT cancelled();
        }
  );

}


