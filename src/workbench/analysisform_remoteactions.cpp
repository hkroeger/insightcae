#include "analysisform.h"
#include "ui_analysisform.h"

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/resultset.h"
#include "base/remoteserverlist.h"
#include "base/wsllinuxserver.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamanalysis.h"
#endif


#include "openfoam/solveroutputanalyzer.h"
#include "remotesync.h"

#ifdef HAVE_WT
#include "remoterun.h"
#endif

#include <QMessageBox>
#include <QDebug>

#include "qsetupremotedialog.h"




namespace fs = boost::filesystem;


void AnalysisForm::connectRemoteActions()
{

  connect(ui->btnDisconnect, &QPushButton::clicked, ui->btnDisconnect,
          [&]()
          {
            if (isRunningRemotely())
              currentWorkbenchAction_.reset();
          }
  );

  connect(ui->btnResume, &QPushButton::clicked,
          this, &AnalysisForm::resumeRemoteRun );

  connect(ui->btnUpload, &QPushButton::clicked,
          this, &AnalysisForm::upload );

  connect(ui->btnDownload, &QPushButton::clicked,
          this, &AnalysisForm::download );

  connect(ui->btnRemoveRemote, &QPushButton::clicked,
          [&]()
          {
            if (!isRunningRemotely())
            {
               if (remoteExecutionConfiguration_)
               {
                 auto answer = QMessageBox::question(
                       this, "Decision required",
                       QString("The directory %1 on server %2 and its contents will be deleted!\nContinue?")
                        .arg(QString::fromStdString(remoteExecutionConfiguration_->location().remoteDir().string()))
                        .arg(QString::fromStdString( *(remoteExecutionConfiguration_->location().serverConfig()) ))
                       );
                 if (answer==QMessageBox::Yes)
                 {
                   remoteExecutionConfiguration_->cleanup(true);
                   delete remoteExecutionConfiguration_;
                 }
              }
            }
            else
            {
              QMessageBox::critical(
                    this, "Not possible",
                    "There is currently a remote analysis running. Please terminate that first!");
            }
          }
  );

}


IQRemoteExecutionState* AnalysisForm::remoteExecutionConfiguration()
{
#ifdef WSL_DEFAULT
  if (isOpenFOAMAnalysis_ && !remoteExecutionConfiguration_ && !remoteExeConfigWasEdited_)
  {
    auto* af = const_cast<AnalysisForm*>(this);
    af->remoteExecutionConfiguration_ = new IQRemoteExecutionState(
          af,
          insight::remoteServers.findFirstServerOfType<insight::WSLLinuxServer>(".*")
          );
  }
#endif
  return remoteExecutionConfiguration_;
}


void AnalysisForm::upload()
{

  remoteExecutionConfiguration()->commit( localCaseDirectory() );

  auto *rstr = new insight::RunSyncToRemote( remoteExecutionConfiguration()->exeConfig() );

  connect(rstr, &insight::RunSyncToRemote::progressValueChanged,
          progressbar_, &QProgressBar::setValue);
  connect(rstr, &insight::RunSyncToRemote::progressTextChanged,
          this,
          [=](const QString& text)
          {
             Q_EMIT statusMessage(text);
          }
  );
  connect(rstr, &insight::RunSyncToRemote::transferFinished,
          this,
          [&]()
          {
            progressbar_->setHidden(true);
            Q_EMIT statusMessage("Transfer to remote location finished");
          }
  );
  connect(rstr, &insight::RunSyncToRemote::transferFinished,
          rstr, &QObject::deleteLater);

  progressbar_->setHidden(false);
  Q_EMIT statusMessage("Transfer to remote location started");

  rstr->start();
  rstr->wait();

}





void AnalysisForm::startRemoteRun()
{
#ifdef HAVE_WT
  remoteExecutionConfiguration()->commit( localCaseDirectory() );
  currentWorkbenchAction_ = RemoteRun::create(this, remoteExecutionConfiguration()->exeConfig(), false);
#endif
}



void AnalysisForm::resumeRemoteRun()
{
#ifdef HAVE_WT
  if (currentWorkbenchAction_)
    throw insight::Exception("Internal error: there is an action running currently!");

  remoteExecutionConfiguration()->commit( localCaseDirectory() );
  currentWorkbenchAction_ = RemoteRun::create(this, remoteExecutionConfiguration()->exeConfig(), true);
#endif
}





void AnalysisForm::download()
{
  auto* rstl = new insight::RunSyncToLocal( remoteExecutionConfiguration()->exeConfig() );

  connect(rstl, &insight::RunSyncToLocal::progressValueChanged,
          progressbar_, &QProgressBar::setValue);
  connect(rstl, &insight::RunSyncToLocal::progressTextChanged,
          this,
          [=](const QString& text)
          {
            Q_EMIT statusMessage(text);
          }
  );
  connect(rstl, &insight::RunSyncToLocal::transferFinished,
          rstl, &QObject::deleteLater);
  connect(rstl, &insight::RunSyncToLocal::transferFinished,
          this,
          [&]()
          {
            progressbar_->setHidden(true);
            Q_EMIT statusMessage("Transfer from remote location to local directory finished");
          }
  );

  progressbar_->setHidden(false);
  Q_EMIT statusMessage("Transfer from remote location to local directory started");

  rstl->start();
  rstl->wait();
}


