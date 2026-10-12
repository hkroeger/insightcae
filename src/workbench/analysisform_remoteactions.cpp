/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
//  *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

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

#include "base/translations.h"


#include "openfoam/solveroutputanalyzer.h"
#include "remotesync.h"

#ifdef HAVE_WT
#include "remoterun.h"
#endif

#include <QApplication>
#include <QMessageBox>
#include <QDebug>

#include "qsetupremotedialog.h"
#include "iqserverlaunchdialog.h"




namespace fs = boost::filesystem;


void AnalysisForm::connectRemoteActions()
{

  connect(ui->btnRemoteConnection, &QPushButton::clicked, this,
          [&]()
          {
            if (isRunningRemotely())
              currentWorkbenchAction_.reset(); // disconnect, run continues remotely
            else
              resumeRemoteRun();
          }
  );

  // the options may be changed during a remote run: pass them to it
  connect(ui->cbDownloadWhenFinished, &QCheckBox::toggled, this,
          [this](bool checked)
          {
#ifdef HAVE_WT
            if (auto *rr = dynamic_cast<RemoteRun*>(currentWorkbenchAction_.data()))
              rr->setDownloadWhenFinished(checked);
#endif
            updateRemoteActionAvailability();
          } );
  connect(ui->cbShutdownWhenFinished, &QCheckBox::toggled, this,
          [this](bool checked)
          {
#ifdef HAVE_WT
            if (auto *rr = dynamic_cast<RemoteRun*>(currentWorkbenchAction_.data()))
              rr->setShutdownWhenFinished(checked);
#endif
          } );

  connect(ui->btnUpload, &QPushButton::clicked,
          this, &AnalysisForm::upload );

  connect(ui->btnDownload, &QPushButton::clicked,
          this, &AnalysisForm::download );

  connect(ui->btnRemoveRemote, &QPushButton::clicked, this,
          [&]()
          {
            if (!isRunningRemotely())
            {
                removeRemoteWorkspace();
            }
            else
            {
              QMessageBox::critical(
                    this, _("Not possible"),
                    _("There is currently a remote analysis running.\n"
                      "Please terminate it first!") );
            }
          }
  );

}





bool AnalysisForm::launchRemoteServerIfRequired()
{
  auto *rs = remoteExecutionConfiguration();
  if (!rs || rs->isCommitted()) return true; // connected: nothing to do

  const auto& scfg = rs->location().serverConfig();
  if (!scfg.isDynamicallyCreatable() || scfg.isRunning())
    return true; // the connection (commit) handles or reports it

  // instead of the silent launch upon commit (blocking the GUI)
  return IQServerLaunchDialog::launchServer(scfg, this);
}




void AnalysisForm::upload()
{
  if (!launchRemoteServerIfRequired()) return;

  remoteExecutionConfiguration()->commit( localCaseDirectory() );

  auto *rstr = new insight::RunSyncToRemote( remoteExecutionConfiguration()->exeConfig(), false );

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
      Q_EMIT statusMessage(_("Transfer to remote location finished"));
          }
  );
  connect(rstr, &insight::RunSyncToRemote::transferFinished,
          rstr, &QObject::deleteLater);

  progressbar_->setHidden(false);
  Q_EMIT statusMessage(_("Transfer to remote location started"));

  rstr->start();
  rstr->wait();

}





void AnalysisForm::startRemoteRun()
{
#ifdef HAVE_WT
  if (!launchRemoteServerIfRequired()) return;

  remoteExecutionConfiguration()->commit( localCaseDirectory() );
  setCurrentWorkbenchAction( RemoteRun::create(this, false) );
#endif
}




void AnalysisForm::resumeRemoteRun()
{
#ifdef HAVE_WT
  if (currentWorkbenchAction_)
      throw insight::Exception(_("Internal error: there is an action running currently!"));

  auto *rs = remoteExecutionConfiguration();
  if (!rs) return;

  if (!rs->isCommitted())
  {
    const auto& scfg = rs->location().serverConfig();
    auto serverLabel = QString::fromStdString(rs->location().serverLabel());

    if (!scfg.isRunning())
    {
      if (!scfg.isDynamicallyCreatable())
      {
        QMessageBox::warning(
              this, _("Remote server not reachable"),
              QString(_("The remote server %1 is not reachable and cannot be launched"
                        " from here (no creation command configured).\n"
                        "Please make sure that it is running and try again."))
                .arg(serverLabel) );
        return;
      }

      auto answer = QMessageBox::question(
            this, _("Remote server not running"),
            QString(_("The remote server %1 is not running.\n"
                      "Shall it be launched?\n\n"
                      "Note: if there was an analysis running on that server before,"
                      " it was most probably terminated, when the server was stopped.\n"
                      "Resuming will then fail, but the remote directory can still be used."))
              .arg(serverLabel) );
      if (answer!=QMessageBox::Yes)
        return;

      // launch with visible console output of the creation command
      if (!IQServerLaunchDialog::launchServer(scfg, this))
        return; // the error was displayed in the dialog
    }

    {
      struct WaitCursor
      {
        WaitCursor() { QApplication::setOverrideCursor(Qt::WaitCursor); }
        ~WaitCursor() { QApplication::restoreOverrideCursor(); }
      } wc;
      Q_EMIT statusMessage(
            QString(_("Connecting to remote server %1...")).arg(serverLabel) );

      // launches the server, if required, and re-evaluates the remote directory.
      // Updates the GUI (incl. remote action buttons).
      // On failure, the remote configuration is left unchanged.
      try
      {
        rs->commit( localCaseDirectory() );
      }
      catch (const std::exception& e)
      {
        Q_EMIT statusMessage(QString());
        QMessageBox::critical(
              this, _("Connection failed"),
              QString(_("Could not connect to the remote server %1:\n%2\n\n"
                        "The remote configuration was kept unchanged."))
                .arg(serverLabel, QString::fromStdString(e.what())) );
        return;
      }

      Q_EMIT statusMessage(QString());
    }

    if (!rs->location().isActive())
    {
      QMessageBox::critical(
            this, _("Remote directory not available"),
            QString(_("The server %1 is up, but the remote directory %2 does not exist.\n"
                      "There is nothing to resume."))
              .arg(serverLabel)
              .arg(QString::fromStdString(rs->location().remoteDir().string())) );
      return;
    }
  }

  setCurrentWorkbenchAction( RemoteRun::create(this, true) );
#endif
}





void AnalysisForm::download()
{
    downloadFromRemote();
}

void AnalysisForm::downloadFromRemote(std::function<void()> completionCallback)
{
  auto* rstl = new insight::RunSyncToLocal( remoteExecutionConfiguration()->exeConfig(), false );

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
          [this,completionCallback]()
          {
            progressbar_->setHidden(true);
      Q_EMIT statusMessage(_("Transfer from remote location to local directory finished"));
            completionCallback();
          }
  );

  progressbar_->setHidden(false);
  Q_EMIT statusMessage(_("Transfer from remote location to local directory started"));

  rstl->start();
  rstl->wait();
}


