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

#include "base/remotelocation.h"
#include "base/remoteexecution.h"
#include "base/translations.h"
#include "openfoam/ofes.h"
#include "base/mountremote.h"

#include "remotedirselector.h"
#include "of_clean_case.h"

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
  QString t = QString::fromStdString(analysisName_);

  QString pf = QString::fromStdString(ist_file_.parent_path().string());

  if (!ist_file_.empty())
    t+=" ("+QString::fromStdString(ist_file_.filename().string());

  if (!pf.isEmpty() && pf!=".")
    t+=" in "+pf;

  t+=")";

  this->setWindowTitle(t);
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





void AnalysisForm::onRunAnalysis()
{
  if (currentWorkbenchAction_)
    throw insight::Exception(_("Internal error: there is an action running currently!"));

  if (!checkAnalysisExecutionPreconditions())
    return;

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
  if (!currentWorkbenchAction_)
    throw insight::Exception(_("Internal error: there is no action running currently!"));

  currentWorkbenchAction_->onCancel();
}




void AnalysisForm::onResultReady(insight::ResultSetPtr results)
{
  resultsViewer_->loadResults(results);

  currentWorkbenchAction_.reset();


  ui->tabWidget->setCurrentWidget(ui->outputTab);


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

