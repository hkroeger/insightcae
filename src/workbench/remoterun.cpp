#include "base/tools.h"
#include "openfoam/remoteexecution.h"

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
  int ret = remote_->execRemoteCmd(
        "analyze "
        "--workdir=\""+remote_->remoteDir().string()+"\" "
        "--server >\""+remote_->remoteDir().string()+"/server.log\" 2>&1 </dev/null &"
      );

  if (ret!=0)
    throw insight::Exception("Failed to launch remote analysis server executable!");
}






RemoteRun::RemoteRun(AnalysisForm *af, bool resume)
  : WorkbenchAction(af),
    resume_(resume)
{
  if (resume)
  {
    remote_.reset
    (
       new insight::RemoteExecutionConfig
       (
         af->ui->localDir->text().toStdString()
       )
    );
  }
  else
  {
    remote_.reset
    (
       new insight::RemoteExecutionConfig
       (
         insight::remoteServers.findServer(
           af->ui->hostList->currentText().toStdString()
           ).second,
         af->ui->localDir->text().toStdString(),
         af->ui->remoteDir->text().toStdString()
       )
    );

  }

  af->ui->remoteDir->setText( QString::fromStdString(remote_->remoteDir().string()) );

  int localPort = insight::findFreePort();
  remote_->createTunnels(
    {},
    { {localPort, "localhost", af_->ui->portNum->value()} }
  );

  ac_.reset(
        new insight::AnalyzeClient(
          str(format("http://localhost:%d") % localPort)
       )
  );

  // lock UI
  af_->ui->label_2->setEnabled(false);
  af_->ui->localDir->setEnabled(false);
  af_->ui->cbDontKeepExeDir->setEnabled(false);
  af_->ui->btnSelectExecDir->setEnabled(false);
  af_->ui->cbRemoteRun->setEnabled(false);
  af_->ui->lbRR1->setEnabled(false);
  af_->ui->hostList->setEnabled(false);
  af_->ui->portNum->setEnabled(false);
  af_->ui->btnResume->setEnabled(false);
  af_->ui->btnDisconnect->setEnabled(false);
  af_->ui->lbRR2->setEnabled(false);
  af_->ui->remoteDir->setEnabled(false);
  af_->ui->btnSelectRemoteDir->setEnabled(false);
  af_->ui->btnUpload->setEnabled(false);
  af_->ui->btnDownload->setEnabled(false);
  af_->ui->btnRemoveRemote->setEnabled(false);

  af_->progdisp_->reset();
  af_->ui->tabWidget->setCurrentWidget(af_->ui->runTab);

  // start
  workerThread_ = boost::thread(
        [this]
        {
          insight::ParameterSet p = af_->parameters_;

          try
          {
            p.packExternalFiles(); // pack
            launchRemoteAnalysisServer();
          }
          catch (...) { exceptionEmitter(); }


          auto monitor = [this]
          {
            std::atomic<bool> runMonitor(true);
            while (runMonitor)
            {
              if (!ac_->isBusy())
                ac_->queryStatus(
                  [this,&runMonitor](bool success, insight::ProgressStatePtrList pis, bool resultsavail)
                  {
                    try {
                    if (!success)
                    {
                      Q_EMIT warning( insight::Exception("Failed to query status!") );
                    }
                    else
                    {
                      if (pis.size()>0)
                      {
                        for (const auto pi: pis)
                        {
                          Q_EMIT analysisProgressUpdate(*pi);
                        }
                      }
                      if (resultsavail)
                      {
                        runMonitor=false;

                        ac_->queryResults(

                              [this](bool success, insight::ResultSetPtr results)
                              {
                                try
                                {
                                  if (!success)
                                  {
                                    Q_EMIT failed( insight::Exception("Failed to fetch results!") );
                                  }
                                  else
                                  {

                                    Q_EMIT finished( results );

                                    ac_->exit(
                                          [this](bool success)
                                          {
                                            try
                                            {
                                              if (!success)
                                              {
                                                Q_EMIT failed( insight::Exception("Failed to stop remote server!") );
                                              }
                                              else
                                              {
                                                remote_->cleanup();
                                              }
                                            }
                                            catch (...) { exceptionEmitter(); }
                                          }
                                    );

                                  }
                                }
                                catch (...) { exceptionEmitter(); }
                              }
                        );
                      }
                    }
                    } catch (...) { exceptionEmitter(); }
                  }
              );
//              else std::cout<<"busy"<<std::endl;



              boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
            }
          };



          if (!resume_)
          {
            std::cout<<"Start"<<std::endl;

            if (!ac_->waitForContact())
              Q_EMIT failed( insight::Exception("Failed to contact analysis server after launch!") );

            ac_->launchAnalysis(
                  p, "/", af_->analysisName_,

                  [this](bool success)
                  {
                    try
                    {
                      if (!success)
                      {
                        Q_EMIT failed( insight::Exception("Failed to launch analysis!") );
                      }
                    }
                    catch (...) { exceptionEmitter(); }
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
  );
}


RemoteRun::~RemoteRun()
{
  cout<<"finish remote run"<<endl;
  workerThread_.join();
  cout<<"worker thread joined"<<endl;
  if (cancelThread_.joinable()) cancelThread_.join();

  // unlock UI
  af_->ui->label_2->setEnabled(true);
  af_->ui->localDir->setEnabled(true);
  af_->ui->cbDontKeepExeDir->setEnabled(true);
  af_->ui->btnSelectExecDir->setEnabled(true);

  af_->ui->cbRemoteRun->setEnabled(true);
  af_->recheckButtonAvailability();
}



insight::RemoteExecutionConfig& RemoteRun::remote()
{
  return *remote_;
}



void RemoteRun::onCancel()
{
  workerThread_.interrupt();

  cancelThread_ = boost::thread(
        [&]()
        {
            // wait for workerThread to end
            while (!workerThread_.try_join_for(boost::chrono::milliseconds(1)))
            {
              boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
            }

            // stop and exit
            ac_->kill(
                  [=](bool success)
                  {
                    if (!success)
                    {
                      Q_EMIT failed( insight::Exception("Failed to stop remote server!") );
                    }
                    else
                    {
                      try {
                       //removeRemoteDirectory();
                      } catch (...) { exceptionEmitter(); }
                      Q_EMIT killed();
                    }
                  }
            );
        }
  );

}
