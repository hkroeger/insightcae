#include "analysisform.h"
#include "ui_analysisform.h"

#include <QMessageBox>

#include "base/remotelocation.h"
#include "base/remoteexecution.h"

namespace fs = boost::filesystem;

void AnalysisForm::updateSaveMenuLabel()
{
  if (act_save_)
  {
    QString packed = pack_parameterset_ ? " (packed)" : "";
    act_save_->setText("&Save parameter set"+packed);
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
  if (currentWorkbenchAction_)
  {
    QMessageBox::critical(this,
                          "Cannot continue",
                          "There is already an action running!");
    return false;
  }

  if (!caseDirectory_)
  {
    QMessageBox::critical(this,
                          "Cannot continue",
                          "Please select a working directory first!");
    return false;
  }

  if (results_)
  {
    QMessageBox msgBox;
    msgBox.setText("There is currently a result set in memory!");
    msgBox.setInformativeText("If you continue, the results will be deleted and the execution directory on disk will be removed (only if it was created). Continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);


    if (msgBox.exec()==QMessageBox::Yes)
    {
        results_.reset();
    } else {
        return false;
    }
  }

  return true;
}


void AnalysisForm::workingDirectoryInputChanged()
{
  QString nwd=ui->leWorkingDirectory->text();
  if (nwd!=lastWorkingDirectory_)
  {
    if (changeWorkingDirectory(nwd))
    {
      lastWorkingDirectory_=nwd;
    }
    else
    {
      ui->leWorkingDirectory->setText(lastWorkingDirectory_);
    }
  }
}


bool AnalysisForm::changeWorkingDirectory(const QString& newWorkingDirectory)
{
  if (remoteDirectory_)
  {
    QMessageBox::critical(this,
                          "Cannot continue",
                          "There is a remote directory connected to the current working directory!\n"
                          "Cannot continue!");
    return false;
  }

  if (currentWorkbenchAction_)
  {
    QMessageBox::critical(this,
                          "Cannot continue",
                          "There is currently an analysis running!\n"
                          "Cannot continue!");
    return false;
  }

  fs::path nwd = newWorkingDirectory.toStdString();

  if (caseDirectory_)
  {
      if (!caseDirectory_->empty() && fs::exists(*caseDirectory_) && !caseDirectory_->keep())
      {
        auto answer = QMessageBox::question(
              this,
              "Reset working directory",
              "There is a ephemeral working directory configured and existing.\n"
              "If you continue, this will be deleted!\n"
              "Do you want to continue?",
              QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
              );

        if (answer!=QMessageBox::Yes)
        {
          return false;
        }
      }
  }


  caseDirectory_.reset(
        new insight::CaseDirectory(nwd)
         );


  if (nwd.empty())
  {
    ui->cbRemoveWorkingDirectory->setChecked( Qt::Checked );
  }
  else if (fs::exists(nwd))
  {
    ui->cbRemoveWorkingDirectory->setChecked( Qt::Unchecked );
  }

  ui->leWorkingDirectory->setToolTip(QString::fromStdString(caseDirectory_->string()));

  if (fs::exists(*caseDirectory_))
  {
    if (boost::filesystem::exists(
          insight::RemoteExecutionConfig::defaultConfigFile(*caseDirectory_))
        )
    {
      Q_EMIT statusMessage("Found remote location config. Validating now...");
      try
      {
        insight::RemoteExecutionConfig rec(*caseDirectory_);
        changeRemoteLocation( &rec );
        ui->gbExecuteOnRemoteHost->setChecked(true);
        ui->ddlExecutionHosts->setCurrentText(QString::fromStdString(remoteDirectory_->server()));
        ui->leRemoteDirectory->setText(QString::fromStdString(remoteDirectory_->remoteDir().string()));
        Q_EMIT statusMessage("Remote location configuration is valid.");

        resumeRemoteRun();
      }
      catch (...)
      {}
    }
  }

  return true;
}



bool AnalysisForm::changeRemoteLocation(const QString& hostLabel, const QString& remoteDir)
{
  if (!hostLabel.isEmpty())
  {
    auto rsi = lookupRemoteServerByLabel(hostLabel);
    insight::RemoteExecutionConfig rec(rsi, *caseDirectory_);
    return changeRemoteLocation(&rec);
  }
  else
    return changeRemoteLocation();
}


bool AnalysisForm::changeRemoteLocation(const insight::RemoteExecutionConfig* rec)
{
  if (!caseDirectory_)
  {
    QMessageBox::critical(
          this,
          "Cannot continue",
          "A local directory needs to be set up first!"
          );
    return false;
  }

  if (remoteDirectory_)
  {
    if (remoteDirectory_->remoteDirExists())
    {
      auto answer = QMessageBox::question(
            this,
            "Decision required",
            "The current analysis is alreadinsight::Exceptiony connected to an existing remote directory.\n"
            "The remote host is: "+QString::fromStdString(remoteDirectory_->server())+"\n"
            "The path on the remote host is:\n"
            +QString::fromStdString(remoteDirectory_->remoteDir().string())+"\n"
            "Do you want to proceed and loose the connection to this remote location?",
            QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
            );
       if (answer!=QMessageBox::Yes)
         return false;
    }
  }

  if (rec)
  {
    remoteDirectory_.reset(
          new insight::RemoteExecutionConfig(*rec)
          );
    if (!ui->gbExecuteOnRemoteHost->isChecked())
      ui->gbExecuteOnRemoteHost->setChecked(true);
  }
  else
  {
    remoteDirectory_.reset();
    if (ui->gbExecuteOnRemoteHost->isChecked())
      ui->gbExecuteOnRemoteHost->setChecked(false);
  }

  return true;
}


void AnalysisForm::applyDirectorySettings()
{
  if (!caseDirectory_)
    throw std::logic_error("Internal error: no local case directory set!");

  caseDirectory_->createDirectory();
}



