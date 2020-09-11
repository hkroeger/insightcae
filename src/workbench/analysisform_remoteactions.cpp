#include "analysisform.h"
#include "ui_analysisform.h"

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/resultset.h"
#include "base/remoteserverlist.h"
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
  connect(ui->btnSetupRemote, &QPushButton::clicked,
          [&]()
          {
            if (!ensureWorkingDirectoryExistence()) return;

            QSetupRemoteDialog dlg(ui->lblHostName->text(), ui->lblRemoteDirectory->text(), this);

            auto btn=dlg.exec();

            if (btn == QDialog::Accepted)
            {
              remoteDirectory_.reset();

              auto rsi = lookupRemoteServerByLabel(dlg.hostName());

              remoteDirectory_.reset(
                    new QRemoteExecutionConfig(
                      this,
                      rsi,
                      *caseDirectory_,
                      dlg.remoteDirectory().toStdString()
                      )
                    );

              ui->lblHostName->setText( QString::fromStdString(remoteDirectory_->server()) );
              ui->lblRemoteDirectory->setText( QString::fromStdString(remoteDirectory_->remoteDir().string() ) );
            }
          }
  );


  connect(ui->leWorkingDirectory, &QLineEdit::editingFinished,
          this, &AnalysisForm::checkForRemoteConfig);

  connect(ui->btnDisconnect, &QPushButton::clicked,
          [&]()
          {
            if (isRunningRemotely())
              currentWorkbenchAction_.reset();
            if (remoteDirectory_)
              remoteDirectory_.reset();
          }
  );

  connect(ui->btnUpload, &QPushButton::clicked,
          this, &AnalysisForm::upload );
  connect(ui->btnDownload, &QPushButton::clicked,
          this, &AnalysisForm::download );

  connect(ui->btnRemoveRemote, &QPushButton::clicked,
          []()
          {
          }
  );

  connect(ui->btnResume, &QPushButton::clicked,
          [&]()
          {
            resumeRemoteRun();
          }
  );
}

void AnalysisForm::checkForRemoteConfig()
{
  if ( caseDirectory_ )
  {
    if (remoteDirectory_)
      remoteDirectory_.reset();

    try
    {

      remoteDirectory_.reset(
            new QRemoteExecutionConfig(
              this,
              *caseDirectory_
              )
            );


      ui->lblHostName->setText( QString::fromStdString(remoteDirectory_->server()) );
      ui->lblRemoteDirectory->setText( QString::fromStdString(remoteDirectory_->remoteDir().string() ) );

    }
    catch (const std::exception& e)
    {
      Q_EMIT statusMessage(e.what());
    }
  }
}

void AnalysisForm::upload()
{
  auto *rstr = new insight::RunSyncToRemote(*remoteDirectory_);

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
  Q_EMIT apply(); // apply all changes into parameter set
  currentWorkbenchAction_.reset(new RemoteRun(this, false));
#endif
}



void AnalysisForm::resumeRemoteRun()
{
#ifdef HAVE_WT
  if (currentWorkbenchAction_)
    throw insight::Exception("Internal error: there is an action running currently!");

  currentWorkbenchAction_.reset(new RemoteRun(this, true));
#endif
}





void AnalysisForm::download()
{
  if (!remoteDirectory_->remoteDirExists())
  {
    throw std::logic_error("The remote directory does not exist! Cannot download.");
  }

  auto* rstl = new insight::RunSyncToLocal(*remoteDirectory_);

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


