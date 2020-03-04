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

#ifdef HAVE_WT
#include "remoterun.h"
#endif

#include <QMessageBox>
#include <QDebug>

namespace bf = boost::filesystem;




std::unique_ptr<insight::MountRemote> AnalysisForm::temporaryMountedRemoteDir() const
{
  std::unique_ptr<insight::MountRemote> result;

  std::string server;
  boost::filesystem::path remoteDir;

  if ( auto *rr = dynamic_cast<RemoteRun*>(currentWorkbenchAction_.get()) )
  {
    server = rr->remote().server();
    remoteDir = rr->remote().remoteDir();
  }
  else
  {
    auto i=insight::remoteServers.find( ui->hostList->currentText().toStdString() );
    if (i->second.isOnDemand())
    {
      throw insight::Exception("Can not mount on-demand instances!");
    }
    server = i->second.server_;
    remoteDir = ui->remoteDir->text().toStdString();
  }

  result.reset(new insight::MountRemote(server, remoteDir));

  return result;
}




void AnalysisForm::upload()
{
  insight::RemoteExecutionConfig rec(currentExecutionPath(false));

  auto *rstr = new insight::RunSyncToRemote(rec);

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

  recheckButtonAvailability();
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


void AnalysisForm::disconnectFromRemoteRun()
{
  recheckButtonAvailability();
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
}


