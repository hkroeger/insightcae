#include "analysisform.h"
#include "ui_analysisform.h"

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/resultset.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/remoteserverlist.h"
#endif


#include "remotedirselector.h"
#include "openfoam/solveroutputanalyzer.h"
#include "remotesync.h"

#include <QMessageBox>

namespace bf = boost::filesystem;



void AnalysisForm::onRemoteServerChanged()
{
}



void AnalysisForm::recheckButtonAvailability()
{
  ui->btnDownload->setEnabled( remoteDownloadOrResumeIsPossible() );
  ui->btnDisconnect->setEnabled( isRunningRemotely() );
  ui->btnResume->setEnabled( remoteDownloadOrResumeIsPossible() && !isRunning() );
  ui->btnRemoveRemote->setEnabled( remoteDownloadOrResumeIsPossible() && !isRunning() );
}




std::unique_ptr<insight::MountRemote> AnalysisForm::temporaryMountedRemoteDir() const
{
  std::unique_ptr<insight::MountRemote> result;


  std::string server = ui->hostList->currentText().toStdString();
  bf::path remote_dir( ui->remoteDir->text().toStdString() );

  auto i = insight::remoteServers.findServer(server);

  {
    bf::path remoteCaseDir =
        i.second.defaultDir_ / boost::filesystem::make_relative(i.second.defaultDir_, remote_dir);

    result.reset(new insight::MountRemote(i.second.serverName_, remoteCaseDir));
  }

  return result;
}




void AnalysisForm::autoSelectRemoteDir()
{
  std::string server = ui->hostList->currentText().toStdString();

  auto i = insight::remoteServers.findServer(server);

  bf::path absloc=bf::canonical(bf::absolute(currentExecutionPath(true)));
  std::string casedirname = absloc.filename().string();

  bf::path remote_dir;
  {
    insight::MountRemote m(i.second.serverName_, i.second.defaultDir_);

    bf::path target_dir = boost::filesystem::unique_path( m.mountpoint()/("isofexecution-"+casedirname+"-%%%%%%%%") );

    remote_dir =
        i.second.defaultDir_ / boost::filesystem::make_relative(m.mountpoint(), target_dir);
  }

  remotePaths_[server]=remote_dir;
  ui->remoteDir->setText( QString::fromStdString(remote_dir.string()) );
}




void AnalysisForm::lockRemoteControls()
{
  ui->cbRemoteRun->setEnabled(false);
  ui->lbRR1->setEnabled(false);
  ui->hostList->setEnabled(false);
  ui->lbRR2->setEnabled(false);
  ui->remoteDir->setEnabled(false);
  ui->btnSelectRemoteDir->setEnabled(false);
}




void AnalysisForm::createRemoteDirectory()
{
  if (ui->remoteDir->text().isEmpty())
    autoSelectRemoteDir();

  std::string server = ui->hostList->currentText().toStdString();
  bf::path remote_dir( ui->remoteDir->text().toStdString() );

  auto i = insight::remoteServers.findServer(server);

  {
    insight::MountRemote m(i.second.serverName_, i.second.defaultDir_);

    bf::path target_dir =
        m.mountpoint() / boost::filesystem::make_relative(i.second.defaultDir_, remote_dir);

    if (!bf::exists(target_dir))
      boost::filesystem::create_directories(target_dir);
  }

  bf::path meta=currentExecutionPath(false)/"meta.foam";
  std::ofstream cfg(meta.c_str());
  cfg << i.second.serverName_ << ":" << remote_dir.string();

  lockRemoteControls();
  recheckButtonAvailability();
}




void AnalysisForm::upload()
{
  if (!isRemoteDirectoryPresent())
    createRemoteDirectory();

  insight::RemoteExecutionConfig rec(currentExecutionPath(false));

  auto *rstr = new insight::RunSyncToRemote(rec);

  connect(rstr, &insight::RunSyncToRemote::progressValueChanged,
          progressbar_, &QProgressBar::setValue);
  connect(rstr, &insight::RunSyncToRemote::progressTextChanged,
          this, [=](const QString& text) { emit statusMessage(text); } );
  connect(rstr, &insight::RunSyncToRemote::transferFinished,
          this, [&]()
  {
    progressbar_->setHidden(true);
    emit statusMessage("Transfer to remote location finished");
  });
  connect(rstr, &insight::RunSyncToRemote::transferFinished,
          rstr, &QObject::deleteLater);

  progressbar_->setHidden(false);
  emit statusMessage("Transfer to remote location started");

  rstr->start();
  rstr->wait();

  recheckButtonAvailability();
}





void AnalysisForm::startRemoteRun()
{
  emit apply(); // apply all changes into parameter set

  // upload
  upload();

  // Remote execution
  std::string server( ui->hostList->currentText().toStdString() );
  bf::path localDir( currentExecutionPath(false) );

  bf::path remoteDir(ui->remoteDir->text().toStdString()); // valid after initRemote!!

  bf::path jobfile_filename;
  if (!ist_file_.empty())
  {
    if (!pack_parameterset_)
      jobfile_filename = ist_file_.filename().stem().string()+"_packed.ist";
    else
      jobfile_filename = ist_file_.filename();
  }
  else
  {
    jobfile_filename = localDir.filename().string()+".ist";
  }

  insight::RemoteExecutionConfig rec(localDir);

  // create jobfile (temporary) and copy to execution dir
  bf::path tempinfile = bf::unique_path( localDir/"remote-input-%%%%-%%%%-%%%%-%%%%.ist" );

  parameters_.packExternalFiles(); // pack
  parameters_.saveToFile(tempinfile, analysisName_); // save with external files embedded
  if (!pack_parameterset_) // if packing is not desired for later ops, remove content again
  {
    parameters_.removePackedData();
  }

#warning Separate thread required
  rec.putFile(tempinfile, jobfile_filename);

  boost::filesystem::remove(tempinfile);

  // start job
  tsi_.reset(new insight::TaskSpoolerInterface( remoteDir/"tsp.socket", server ) );
  tsi_->startJob({
          "analyze",
          "--workdir", remoteDir.string(),
          (remoteDir/jobfile_filename).string()
  });

  resumeRemoteRun();

}




void AnalysisForm::updateOutputAnalzer(QString line)
{
  soa_->update(line.toStdString());
}




void AnalysisForm::resumeRemoteRun()
{
  insight::RemoteExecutionConfig rec(currentExecutionPath(false));

  if (!tsi_)
    tsi_.reset(new insight::TaskSpoolerInterface( rec.remoteDir()/"tsp.socket", rec.server() ) );

  if (!tsi_->isTailRunning())
  {
    soa_.reset(new insight::SolverOutputAnalyzer(*progdisp_)); // reset
    connect(this, &AnalysisForm::logReady, this, &AnalysisForm::updateOutputAnalzer );
    connect(this, &AnalysisForm::logReady, log_, &LogViewerWidget::appendLine );

    tsi_->startTail(
          [&](std::string line) {
            emit logReady(QString::fromStdString(line));
          }
    );
  }

  recheckButtonAvailability();
}


void AnalysisForm::disconnectFromRemoteRun()
{
  if (tsi_)
  {
    tsi_->stopTail();
    tsi_.reset();
    soa_.reset();
  }

  recheckButtonAvailability();
}



void AnalysisForm::stopRemoteRun()
{
  std::string server( ui->hostList->currentText().toStdString() );
  bf::path localDir( currentExecutionPath(false) );

  if ( isRemoteDirectoryPresent() && isRunningRemotely() )
  {

    bf::path remoteDir(ui->remoteDir->text().toStdString()); // valid after initRemote!!
//    insight::TaskSpoolerInterface tsi( remoteDir/"tsp.socket", server );

    tsi_->cancelAllJobs();

    tsi_->stopTail();
    tsi_.reset();
    soa_.reset();
  }

  recheckButtonAvailability();
}




void AnalysisForm::unlockRemoteControls()
{
  ui->cbRemoteRun->setEnabled(true);
  ui->lbRR1->setEnabled(true);
  ui->hostList->setEnabled(true);
  ui->lbRR2->setEnabled(true);
  ui->remoteDir->setEnabled(true);
  ui->btnSelectRemoteDir->setEnabled(true);
}



void AnalysisForm::download()
{
  if (isRemoteDirectoryPresent())
  {
    insight::RemoteExecutionConfig rec(currentExecutionPath(false));

    auto* rstl = new insight::RunSyncToLocal(rec);

    connect(rstl, &insight::RunSyncToLocal::progressValueChanged,
            progressbar_, &QProgressBar::setValue);
    connect(rstl, &insight::RunSyncToLocal::progressTextChanged,
            this, [=](const QString& text) { emit statusMessage(text); } );
    connect(rstl, &insight::RunSyncToLocal::transferFinished,
            rstl, &QObject::deleteLater);
    connect(rstl, &insight::RunSyncToLocal::transferFinished,
            this, [&]()
                  {
                    progressbar_->setHidden(true);
                    emit statusMessage("Transfer from remote location to local directory finished");
                  }
    );

    progressbar_->setHidden(false);
    emit statusMessage("Transfer from remote location to local directory started");

    rstl->start();
    rstl->wait();
  }
}




void AnalysisForm::removeRemoteDirectory()
{
  if (isRemoteDirectoryPresent())
  {
    insight::RemoteExecutionConfig rec(currentExecutionPath(false));

    auto answer = QMessageBox::critical
        ( this,
          "Please confirm",
          "The remote run on host "+QString::fromStdString(rec.server())+
          " in directory \""+QString::fromStdString(rec.remoteDir().string())+
          "\" will be stopped and the remote directory will be deleted!\n"+
          "This can not be undone.\n"+
          "Please make sure that you downloaded the case, if you want to keep the results!\n"+
          "\nProceed and remove the remote directory?",
          QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
          QMessageBox::No);

    if (answer == QMessageBox::Yes)
    {
      if (tsi_ && tsi_->isTailRunning())
      {
        // cancel
        stopRemoteRun();
      }

      rec.removeRemoteDir();
      boost::filesystem::remove( currentExecutionPath(false)/"meta.foam" );

      unlockRemoteControls();

      recheckButtonAvailability();
    }
  }
}
