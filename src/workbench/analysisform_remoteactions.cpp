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
                        .arg(QString::fromStdString(remoteExecutionConfiguration_->remoteDir().string()))
                        .arg(QString::fromStdString( *(remoteExecutionConfiguration_->serverConfig()) ))
                       );
                 if (answer==QMessageBox::Yes)
                 {
                   remoteExecutionConfiguration_->cleanup();
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


insight::RemoteExecutionConfig* AnalysisForm::remoteExecutionConfiguration() const
{
#ifdef WSL_DEFAULT
  if (!remoteExecutionConfiguration_)
  {
    auto* af = const_cast<AnalysisForm*>(this);
    af->remoteExecutionConfiguration_ = new QRemoteExecutionState(
          af,
          insight::remoteServers.findFirstServerOfType<insight::WSLLinuxServer>(".*"),
          localCaseDirectory()
          );
  }
#endif
  return remoteExecutionConfiguration_;
}


void AnalysisForm::upload()
{

  auto *rstr = new insight::RunSyncToRemote( *remoteExecutionConfiguration() );

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
  currentWorkbenchAction_ = RemoteRun::create(this, *remoteExecutionConfiguration(), false);
#endif
}



void AnalysisForm::resumeRemoteRun()
{
#ifdef HAVE_WT
  if (currentWorkbenchAction_)
    throw insight::Exception("Internal error: there is an action running currently!");

  currentWorkbenchAction_ = RemoteRun::create(this, *remoteExecutionConfiguration(), true);
#endif
}





void AnalysisForm::download()
{
  auto* rstl = new insight::RunSyncToLocal(*remoteExecutionConfiguration());

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


