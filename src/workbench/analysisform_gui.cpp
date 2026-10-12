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

#include <QMessageBox>
#include <QProcess>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#include "base/remotelocation.h"
#include "base/remoteexecution.h"
#include "base/translations.h"
#include "openfoam/ofes.h"
#include "base/mountremote.h"

#include "remotedirselector.h"
#include "of_clean_case.h"
#include "base/warningdispatcher.h"
#include "base/wsllinuxserver.h"

namespace fs = boost::filesystem;

void AnalysisForm::updateSaveMenuLabel()
{
  if (act_save_)
  {
        QString packed = pack_parameterset_ ? (QString(" ")+_("(packed)")) : "";
      act_save_->setText(_("&Save parameter set")+packed);
  }
  if (act_pack_)
  {
    act_pack_->setChecked(pack_parameterset_);
  }
}




void AnalysisForm::updateWindowTitle()
{
    QString newTitle =
        QString::fromStdString(psmodel_->getAnalysisName());

    if (isModified())
    {
        newTitle = "*"+newTitle;
    }

    if (currentFileNameIsSet())
    {
        auto istFile=currentFileName();

        newTitle+=" ("+QString::fromStdString(
                 istFile.filename().string());

        QString pf = QString::fromStdString(
            istFile.parent_path().string());

        if (!pf.isEmpty() && pf!=".")
            newTitle+=" in "+pf;

        newTitle+=")";
    }

    this
        ->topLevelWidget()
        ->setWindowTitle(newTitle);
}



bool AnalysisForm::checkAnalysisExecutionPreconditions()
{
  if (resultsViewer_->hasResults())
  {
    QMessageBox msgBox;
    msgBox.setText(_("There is currently a result set in memory!"));
    msgBox.setInformativeText(
        _("If you continue, the results will be deleted and "
          "the execution directory on disk will be removed (only if it was created). Continue?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);


    if (msgBox.exec()==QMessageBox::Yes)
    {
        resultsViewer_->clear();
    }
    else
    {
        return false;
    }
  }

  return true;
}




bool AnalysisForm::inputDataHasErrors() const
{
    if (inputDataPending_) return false;
    return std::any_of(
        inputDataIssues_.begin(), inputDataIssues_.end(),
        [](const insight::InputDataIssue& i)
        { return i.severity==insight::InputDataIssue::Error; } );
}


insight::InputDataIssueList AnalysisForm::allIssues() const
{
    auto r = inputDataPending_ ? insight::InputDataIssueList() : inputDataIssues_;
    r.insert(r.end(), otherIssues_.begin(), otherIssues_.end());
    return r;
}


void AnalysisForm::updateRunAvailability()
{
    bool runPossible = !currentWorkbenchAction_ && !inputDataHasErrors();
    ui->btnRun->setEnabled(runPossible);
    if (act_run_) act_run_->setEnabled(runPossible);

    updateRemoteActionAvailability();
}




void AnalysisForm::updateRemoteActionAvailability(const IQRemoteExecutionState* rs)
{
    bool hasRemote = (rs!=nullptr);
    // committed and remote directory verified (server up, directory existing)
    bool committed = hasRemote && rs->isCommitted() && rs->location().isActive();
    bool isWSL = hasRemote &&
        dynamic_cast<const insight::WSLLinuxServer::Config*>(
            &rs->location().serverConfig() );

    // connect/disconnect
    bool connected = isRunningRemotely();
    // a temporary directory, which was not yet created, contains nothing to resume
    bool resumable = hasRemote && !rs->location().remoteDir().empty();
    ui->btnRemoteConnection->setEnabled(
        hasRemote && !isWSL
        && (connected || resumable)
        && !isRunningLocally() && !waitingForInputPreprocessing_ );
    if (connected)
    {
        ui->btnRemoteConnection->setText(_("Disconnect"));
        ui->btnRemoteConnection->setToolTip(
            _("Detach from the running remote analysis. It continues on the server."));
    }
    else
    {
        ui->btnRemoteConnection->setText(_("Resume"));
        ui->btnRemoteConnection->setToolTip(
            _("Reconnect to an analysis running in the remote location."));
    }

    // upload commits the remote location itself
    ui->btnUpload->setEnabled(
        hasRemote && !isRunning() && !waitingForInputPreprocessing_ );

    // download is possible during a remote run (intermediate results)
    ui->btnDownload->setEnabled(
        committed && !isRunningLocally() );

    // without commit, nothing exists in the remote location
    ui->btnRemoveRemote->setEnabled(
        committed && !isRunning() && !waitingForInputPreprocessing_ );

    // the options may be changed during a remote run,
    // until the respective step has started
    bool downloadDecided = false, shutdownStarted = false;
#ifdef HAVE_WT
    if (auto *rr = dynamic_cast<RemoteRun*>(currentWorkbenchAction_.data()))
    {
        downloadDecided = rr->downloadStepStarted();
        shutdownStarted = rr->shutdownStepStarted();
    }
#endif

    ui->cbDownloadWhenFinished->setEnabled( hasRemote && !downloadDecided );
    ui->cbDownloadWhenFinished->setToolTip(
        !hasRemote ? QString() :
        downloadDecided ? _("The download has started already or was skipped.") :
        _("Download the results into the local directory, when the remote analysis has finished.") );

    // shut down the server after completion: only possible with a destruction command.
    // (Performed after the result set has been retrieved and the
    // files have been downloaded, if selected)
    bool destructable = hasRemote && rs->location().serverConfig().isDynamicallyDestructable();
    if (!destructable && !shutdownStarted)
        ui->cbShutdownWhenFinished->setChecked(false);
    ui->cbShutdownWhenFinished->setEnabled(
        destructable && !shutdownStarted );
    ui->cbShutdownWhenFinished->setToolTip(
        !hasRemote ? QString() :
        shutdownStarted ? _("The shutdown is in progress.") :
        !destructable ? _("The server cannot be shut down from here (no destruction command configured).") :
        _("Shut down the server (execute its destruction command), once the result set"
          " has been retrieved (and the files have been downloaded, if selected).\n"
          "Files in the remote directory, which are not downloaded, might be lost.") );
}




void AnalysisForm::updateRemoteActionAvailability()
{
    updateRemoteActionAvailability(remoteExecutionConfiguration());
}




void AnalysisForm::setCurrentWorkbenchAction(WorkbenchAction* a)
{
    currentWorkbenchAction_.reset(a);
    updateRemoteActionAvailability();
}


void AnalysisForm::updateIssueIndication()
{
    auto issues = allIssues();

    // traffic light
    if (trafficLight_)
    {
        if (inputDataPending_)
        {
            trafficLight_->setState(IQTrafficLight::Unknown);
            trafficLight_->setToolTip(tr("Checking input data..."));
        }
        else if (inputDataHasErrors())
        {
            trafficLight_->setState(IQTrafficLight::Red);
            trafficLight_->setToolTip(tr("The input data contains errors. The analysis cannot be run."));
        }
        else if (!issues.empty())
        {
            trafficLight_->setState(IQTrafficLight::Yellow);
            trafficLight_->setToolTip(tr("There are warnings. Please review."));
        }
        else
        {
            trafficLight_->setState(IQTrafficLight::Green);
            trafficLight_->setToolTip(tr("Input data ok"));
        }
    }

    // review button
    if (btnReview_)
        btnReview_->setVisible(!issues.empty());

    // review dialog, if open
    if (issueListWidget_)
    {
        issueListWidget_->clear();
        for (const auto& i: issues)
        {
            auto *item = new QListWidgetItem(
                style()->standardIcon(
                    i.severity==insight::InputDataIssue::Error ?
                        QStyle::SP_MessageBoxCritical : QStyle::SP_MessageBoxWarning ),
                QString::fromStdString(i.source+": "+i.message),
                issueListWidget_ );
            item->setToolTip(QString::fromStdString(i.message));
        }
    }

    updateRunAvailability();
}


void AnalysisForm::clearOtherIssues()
{
    otherIssues_.clear();
    insight::WarningDispatcher::getCurrent().clear();
    updateIssueIndication();
}


void AnalysisForm::onInputDataPending()
{
    inputDataPending_=true;
    inputDataIssues_.clear();
    otherIssues_.clear();
    updateIssueIndication();
}


void AnalysisForm::onInputDataIssuesChanged(insight::InputDataIssueList issues)
{
    inputDataPending_=false;
    inputDataIssues_=issues;
    updateIssueIndication();
}


void AnalysisForm::onReviewIssues()
{
    if (issueDialog_)
    {
        issueDialog_->raise();
        issueDialog_->activateWindow();
        return;
    }

    auto *dlg = new QDialog(this);
    issueDialog_ = dlg;
    dlg->setWindowTitle(tr("Warnings and Errors"));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->resize(700, 350);

    auto *layout = new QVBoxLayout(dlg);
    issueListWidget_ = new QListWidget(dlg);
    issueListWidget_->setWordWrap(true);
    layout->addWidget(issueListWidget_);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
    connect(btnBox, &QDialogButtonBox::rejected, dlg, &QDialog::close);
    layout->addWidget(btnBox);

    connect(dlg, &QDialog::destroyed, this, [this]() {
        issueListWidget_ = nullptr;
    });

    updateIssueIndication(); // fills the list
    dlg->show();
}


void AnalysisForm::onRunAnalysis()
{
  if (waitingForInputPreprocessing_)
    return; // run will start, once preprocessing has finished

  if (currentWorkbenchAction_)
    throw insight::Exception(_("Internal error: there is an action running currently!"));

  if (inputDataHasErrors())
    return; // red light: run is not possible

  if (!checkAnalysisExecutionPreconditions())
    return;

  clearOtherIssues();

  if (remoteExecutionConfiguration())
  {
    startRemoteRun();
  }
  else
  {
    startLocalRun();
  }
}




void AnalysisForm::onKillAnalysis()
{
  if (waitingForInputPreprocessing_)
  {
    // run not yet started: just don't start it
    peditor_->cancelWaitForVisualization();
    waitingForInputPreprocessing_=false;
    updateRemoteActionAvailability();
    Q_EMIT statusMessage(_("Analysis start cancelled"), 5000);
    return;
  }

  if (!currentWorkbenchAction_)
    throw insight::Exception(_("Internal error: there is no action running currently!"));

  currentWorkbenchAction_->onCancel();
}




void AnalysisForm::onResultReady()
{
  ui->tabWidget->setCurrentWidget(ui->outputTab);
    resultsViewer_->loadResults(
      currentWorkbenchAction_->moveResults() );

  currentWorkbenchAction_.reset();




  QMessageBox::information(
      this, _("Finished!"),
      _("The analysis has finished") );

}

void AnalysisForm::onAnalysisError(std::__exception_ptr::exception_ptr e)
{
  currentWorkbenchAction_.reset();
  if (e) std::rethrow_exception(e);
}




void AnalysisForm::onAnalysisCancelled()
{
  currentWorkbenchAction_.reset();
  QMessageBox::information(
      this, _("Stopped!"),
      _("The analysis has been interrupted upon user request!"));
}


void AnalysisForm::cleanFinishedExternalProcesses()
{
    auto rps=externalProcesses_;
    externalProcesses_.clear();
    std::copy_if(
        rps.begin(), rps.end(),
        std::inserter(externalProcesses_, externalProcesses_.begin()),
        [](insight::JobPtr rp)
        {
            return rp && rp->isRunning();
        }
    );
}

void AnalysisForm::onStartPV()
{
  bool launchRemote=false;
  if (auto* rec = remoteExecutionConfiguration())
  {
      if (rec->isCommitted()) launchRemote=true;
  }

  if (launchRemote)
      onStartPVRemote();
  else
      onStartPVLocal();
}



void AnalysisForm::onStartPVLocal()
{
    IQParaviewDialog dlg( localCaseDirectory(), this );
    dlg.exec();

    if (auto pv = dlg.paraviewProcess())
    {
        cleanFinishedExternalProcesses(); // clean up on this occasion
        externalProcesses_.insert(pv);
    }
}


void AnalysisForm::onStartPVRemote()
{
    if (auto* rec = remoteExecutionConfiguration())
    {
        if (rec->location().remoteDirExists())
        {
            IQRemoteParaviewDialog dlg( rec->exeConfig(), this );
            dlg.exec();

            if (auto rp = dlg.remoteParaviewProcess())
            {
                cleanFinishedExternalProcesses(); // clean up on this occasion
                externalProcesses_.insert(rp);
            }
        }
    }
}



void AnalysisForm::onCleanOFC()
{
#ifndef WIN32
  const insight::OFEnvironment* ofc = nullptr;
  if (parameters().contains("run/OFEname"))
  {
    std::string ofename=parameters().getString("run/OFEname");
    ofc=&(insight::OFEs::get(ofename));
  }
  else
  {
    ofc=&(insight::OFEs::getCurrentOrPreferred());
  }

  fs::path exePath = localCaseDirectory();
  std::unique_ptr<insight::MountRemote> rd;
  if (auto* rec = remoteExecutionConfiguration())
  {
      rd.reset(new insight::MountRemote(rec->location()));
      exePath = rd->mountpoint();
  }

  OFCleanCaseDialog dlg(*ofc, exePath, this);
  dlg.exec();
#endif
}




void AnalysisForm::onWnow()
{
    if (isRunningLocally())
    {
      fs::path exePath = localCaseDirectory();
      std::ofstream f( (exePath/"wnow").string() );
      f.close();
    }
    else if (isRunningRemotely())
    {
        if (auto *rr = dynamic_cast<RemoteRun*>(currentWorkbenchAction_.get()))
        {
            rr->analyzeClient().wnow(
                        [](insight::AnalyzeClientAction::ReportSuccessResult) {},
                        []() {}
                        );
        }
    }
}




void AnalysisForm::onWnowAndStop()
{
    if (isRunningLocally())
    {
      fs::path exePath = localCaseDirectory();
      std::ofstream f( (exePath/"wnowandstop").string() );
      f.close();
    }
    else if (isRunningRemotely())
    {
        if (auto *rr = dynamic_cast<RemoteRun*>(currentWorkbenchAction_.get()))
        {
            rr->analyzeClient().wnowandstop(
                        [](insight::AnalyzeClientAction::ReportSuccessResult) {},
                        []() {}
                        );
        }
    }
}




void AnalysisForm::onShell()
{
  auto locDir = QString::fromStdString(localCaseDirectory().string());

  if ( auto* rec = remoteExecutionConfiguration() )
  {
    if (rec->location().remoteDirExists())
    {
      QStringList args;
      if ( !QProcess::startDetached("isRemoteShell.sh",
                                    args, locDir)
           )
      {
        QMessageBox::critical(
                    this, _("Failed to start"),
                    QString(_("Failed to start remote shell in directory %1")).arg(locDir)
              );
      }
    }
  }
  else
  {
    QStringList args;
    args << "--working-directory" << locDir;
    if (!QProcess::startDetached("mate-terminal", args, locDir ))
    {
      QMessageBox::critical(
          this, _("Failed to start"),
          QString(_("Failed to start mate-terminal in directory %1")).arg(locDir)
          );
    }
  }
}

