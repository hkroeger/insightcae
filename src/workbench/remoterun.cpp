#include "remoterun.h"
#include "analysisform.h"
#include "ui_analysisform.h"

#include "openfoam/remoteexecution.h"

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include "boost/process.hpp"

namespace bf=boost::filesystem;
namespace bp=boost::process;

void RemoteRun::createRemoteDirectory()
{
  std::string server = af_->ui->hostList->currentText().toStdString();
  bf::path target_dir, remote_dir( af_->ui->remoteDir->text().toStdString() );
  auto i = insight::remoteServers.findServer(server);

  insight::MountRemote m(i.second.serverName_, i.second.defaultDir_);

  if (remote_dir.empty())
  {
    std::string casename=af_->analysisName_;
    casename.erase(
          remove_if(casename.begin(), casename.end(), [](const char c) { return !isalnum(c); } ),
          casename.end());

    if (!af_->ist_file_.empty())
      casename = af_->ist_file_.filename().string();


    target_dir = boost::filesystem::unique_path( m.mountpoint()/("isofexecution-"+casename+"-%%%%%%%%") );

    boost::filesystem::create_directories(target_dir);

    remote_dir =
        i.second.defaultDir_ / boost::filesystem::make_relative(m.mountpoint(), target_dir);

    af_->ui->remoteDir->setText( QString::fromStdString(remote_dir.string()) );
  }
  else
  {
    target_dir =  m.mountpoint() / boost::filesystem::make_relative(i.second.defaultDir_, remote_dir);
  }

  if (!bf::exists(target_dir))
    throw insight::Exception("Remote directory does not exist!");

}




void RemoteRun::launchRemoteAnalysisServer()
{
  std::string server = af_->ui->hostList->currentText().toStdString();
  bf::path remoteDir( af_->ui->remoteDir->text().toStdString() );
  auto i = insight::remoteServers.findServer(server);

  int ret = bp::system
      (
        bp::search_path("ssh"),
        bp::args( { i.second.serverName_, "nohup analyze --workdir=\""+remoteDir.string()+"\" --server >/dev/null 2>&1 &" } )
        );

  if (ret!=0)
    throw insight::Exception("Failed to launch remote analysis server executable!");
}




void RemoteRun::removeRemoteDirectory()
{
  std::string server = af_->ui->hostList->currentText().toStdString();
  bf::path remoteDir( af_->ui->remoteDir->text().toStdString() );
  auto i = insight::remoteServers.findServer(server);

  insight::RemoteLocation(i.second.serverName_, remoteDir).removeRemoteDir();
}




RemoteRun::RemoteRun(AnalysisForm *af, bool resume)
  : WorkbenchAction(af),
    ac_( "http://"
         +af_->ui->hostList->currentText().toStdString()
         +":"
         +QString::number(af_->ui->portNum->value()).toStdString()
         )
{
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
        [&]()
        {
          insight::ParameterSet p = af_->parameters_;

          try
          {
            p.packExternalFiles(); // pack

//            createRemoteDirectory();
//            launchRemoteAnalysisServer();
          }
          catch (...) { exceptionEmitter(); }

          auto monitor = [&]()
          {
            bool resultsFetched=false;
            while (!resultsFetched)
            {
              if (!ac_.isBusy()) ac_.queryStatus(
                  [&](bool success, insight::ProgressStatePtr pi, bool resultsavail)
                  {
                    try {
                    if (!success)
                    {
                      Q_EMIT warning( insight::Exception("Failed to query status!") );
                    }
                    else
                    {
                      if (pi)
                      {
                        Q_EMIT analysisProgressUpdate(*pi);
                      }
                      if (resultsavail)
                      {
                        ac_.queryResults(
                              [&](bool success, insight::ResultSetPtr results)
                              {
                                try {
                                if (!success)
                                {
                                  Q_EMIT failed( insight::Exception("Failed to fetch results!") );
                                }
                                else
                                {
                                  resultsFetched=true;

//                                  ac_.exit(
//                                        [=](bool success)
//                                        {
//                                          try {
//                                          if (!success)
//                                          {
//                                            Q_EMIT failed( insight::Exception("Failed to stop remote server!") );
//                                          }
//                                          else
//                                          {
                                            Q_EMIT finished( results );
//                                          }
//                                          } catch (...) { exceptionEmitter(); }
//                                        }
//                                  );

                                  try
                                  {
                                    //removeRemoteDirectory();
                                  } catch (...) { exceptionEmitter(); }
                                }
                                } catch (...) { exceptionEmitter(); }
                              }
                        );
                      }
                    }
                    } catch (...) { exceptionEmitter(); }
                  }
              );
              else std::cout<<"busy"<<std::endl;



              boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
            }
          };

          if (resume)
          {
            monitor();
          }
          else
          {
            ac_.launchAnalysis(
                  p, "/", af_->analysisName_,

                  [&](bool success)
                  {
                    try {
                    if (!success)
                    {
                      Q_EMIT failed( insight::Exception("Failed to launch analysis!") );
                    }
                    else
                    {
                      monitor();
                    }
                    } catch (...) { exceptionEmitter(); }
                  }
            );
          }
        }
  );
}


RemoteRun::~RemoteRun()
{
  workerThread_.join();
  if (cancelThread_.joinable()) cancelThread_.join();

  // unlock UI
  af_->ui->label_2->setEnabled(true);
  af_->ui->localDir->setEnabled(true);
  af_->ui->cbDontKeepExeDir->setEnabled(true);
  af_->ui->btnSelectExecDir->setEnabled(true);

  af_->ui->cbRemoteRun->setEnabled(true);
  af_->recheckButtonAvailability();
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
            ac_.kill(
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
