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





void AnalysisForm::upload()
{

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
  remoteExecutionConfiguration()->commit( localCaseDirectory() );
  currentWorkbenchAction_.reset( RemoteRun::create(this, false) );
#endif
}




void AnalysisForm::resumeRemoteRun()
{
#ifdef HAVE_WT
  if (currentWorkbenchAction_)
      throw insight::Exception(_("Internal error: there is an action running currently!"));

  remoteExecutionConfiguration()->commit( localCaseDirectory() );
  currentWorkbenchAction_.reset( RemoteRun::create(this, true) );
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


