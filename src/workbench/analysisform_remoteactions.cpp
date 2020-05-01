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


#include "remotedirselector.h"
#include "openfoam/solveroutputanalyzer.h"
#include "remotesync.h"

#ifdef HAVE_WT
#include "remoterun.h"
#endif

#include <QMessageBox>
#include <QDebug>





namespace fs = boost::filesystem;





void AnalysisForm::upload()
{
  if (!caseDirectory_)
  {
    throw std::logic_error("Internal error: no case directory configured!");
  }

  if (!remoteDirectory_)
  {
    throw std::logic_error("Internal error: attempt to upload but no remote directory configured!");
  }

  applyDirectorySettings();

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
  applyDirectorySettings();

#ifdef HAVE_WT
  Q_EMIT apply(); // apply all changes into parameter set
  currentWorkbenchAction_.reset(new RemoteRun(this, false));
#endif
}



void AnalysisForm::resumeRemoteRun()
{
  applyDirectorySettings();

#ifdef HAVE_WT
  if (currentWorkbenchAction_)
    throw insight::Exception("Internal error: there is an action running currently!");

  currentWorkbenchAction_.reset(new RemoteRun(this, true));
#endif
}


void AnalysisForm::disconnectFromRemoteRun()
{
}




void AnalysisForm::download()
{
  if (!caseDirectory_)
  {
    throw std::logic_error("Internal error: no case directory configured!");
  }

  if (!remoteDirectory_)
  {
    throw std::logic_error("Internal error: attempt to upload but no remote directory configured!");
  }

  if (!remoteDirectory_->remoteDirExists())
  {
    throw std::logic_error("The remote directory does not exist! Cannot download.");
  }

  applyDirectorySettings();

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


