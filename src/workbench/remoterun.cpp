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



void RemoteRun::launchRemoteAnalysisServer()
{
  auto rd = remote_.remoteDir();

  int ret = remote_.execRemoteCmd(
        "analyze "
        "--workdir=\""+rd.string()+"\" "
        "--server >\""+rd.string()+"/server.log\" 2>&1 </dev/null &"
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
//  int localPort = insight::findFreePort();

  portMappings_ = remote_.server()->makePortsAccessible(
      {8090},
      {}
//    {},
//    { {localPort, "localhost", /*af_->ui->portNum->value()*/8090 } }
  );

  af_->progressDisplayer_.reset();
  af_->ui->tabWidget->setCurrentWidget(af_->ui->runTab);

  ac_.reset(
        new insight::AnalyzeClient(
          af_->analysisName_,
          str(format("http://localhost:%d") % /*localPort*/portMappings_->localListenerPort(8090) ),
          &af_->progressDisplayer_,
          [this](std::exception_ptr e)
            {
              workerThread_->interrupt();
              Q_EMIT failed(e);
            }
       )
  );


  // start
  workerThread_.reset( new insight::QAnalysisThread(
        [this]
        {

          insight::ParameterSet p = af_->parameters();

          p.packExternalFiles(); // pack
          launchRemoteAnalysisServer();


          auto monitor = [this]
          {
            std::atomic<bool> runMonitor(true);

            while (runMonitor)
            {
              if (!ac_->isBusy())
              {
                ac_->queryStatus(
                  [this,&runMonitor](bool success, bool resultsavail)
                  {
                    if (!success)
                    {
                      Q_EMIT statusMessage( "Failed to query status!" );
                    }
                    else
                    {
                      if (resultsavail)
                      {
                        runMonitor=false;

                        ac_->queryResults(

                            [this](bool success, insight::ResultSetPtr results)
                            {
                                if (!success)
                                {
                                  throw insight::Exception("Failed to fetch results!");
                                }
                                else
                                {
                                  Q_EMIT finished( results );

                                  ac_->exit(
                                        [this](bool success)
                                        {
                                          if (!success)
                                          {
                                            throw insight::Exception("Failed to stop remote server!");
                                          }
                                          else
                                          {
                                            remote_.cleanup();
                                          }
                                        }
                                  );
                                }
                            }
                        );
                      }
                    }
                  }
                );
              }

              boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
            }
          };



          if (!resume_)
          {

            if (!ac_->waitForContact())
              throw insight::Exception("Failed to contact analysis server after launch!");

            ac_->launchAnalysis(
                p, "/", af_->analysisName_,

                [](bool success)
                {
                  if (!success) throw insight::Exception("Failed to launch analysis!");
                }
            );


            std::cout<<"Connection established."<<std::endl;

          }
          else
          {
            std::cout<<"Resume"<<std::endl;
          }

          monitor();
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
            ac_->kill(
                  [=](bool success)
                  {
                    if (!success)
                    {
                      Q_EMIT failed( std::make_exception_ptr(insight::Exception("Failed to stop remote server!") ) );
                    }
                    else
                    {
                      Q_EMIT cancelled();
                    }
                  }
            );
        }
  );

}
