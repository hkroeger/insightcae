#include "analysisform.h"
#include "ui_analysisform.h"

#include <QMessageBox>
#include <QProcess>

#include "base/remotelocation.h"
#include "base/remoteexecution.h"
#include "openfoam/ofes.h"

#include "remoteparaview.h"
#include "remotedirselector.h"
#include "of_clean_case.h"

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
    throw insight::Exception("Internal error: there is an action running currently!");

  if (!checkAnalysisExecutionPreconditions())
    return;

  if (remoteExecutionConfiguration_)
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
    throw insight::Exception("Internal error: there is no action running currently!");

  currentWorkbenchAction_->onCancel();
}




void AnalysisForm::onResultReady(insight::ResultSetPtr results)
{
  results_=results;

  currentWorkbenchAction_.reset();

  resultsModel_=new insight::QResultSetModel(results_, ui->resultsToC);
  ui->resultsToC->setModel(resultsModel_);
  ui->resultsToC->expandAll();
  ui->resultsToC->resizeColumnToContents(0);
  ui->resultsToC->resizeColumnToContents(1);

  ui->tabWidget->setCurrentWidget(ui->outputTab);


  QMessageBox::information(this, "Finished!", "The analysis has finished");

}

void AnalysisForm::onAnalysisError(std::__exception_ptr::exception_ptr e)
{
  currentWorkbenchAction_.reset();
  if (e) std::rethrow_exception(e);
}




void AnalysisForm::onAnalysisCancelled()
{
  currentWorkbenchAction_.reset();
  QMessageBox::information(this, "Stopped!", "The analysis has been interrupted upon user request!");
}




void AnalysisForm::onStartPV()
{
  if (auto* rec = remoteExecutionConfiguration())
  {
    if (rec->remoteDirExists())
    {
      RemoteParaview dlg( *rec, this );
      dlg.exec();
    }
  }
  else
  {
    ::system( boost::str( boost::format
          ("cd %s; isPV.py &" ) % localCaseDirectory().string()
     ).c_str() );
  }
}




void AnalysisForm::onCleanOFC()
{
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
      rd.reset(new insight::MountRemote(*rec));
      exePath = rd->mountpoint();
  }

  OFCleanCaseDialog dlg(*ofc, exePath, this);
  dlg.exec();
}




void AnalysisForm::onWnow()
{
  if (isRunning())
  {
    fs::path exePath = localCaseDirectory();
    std::unique_ptr<insight::MountRemote> rd;
    if ( auto* rec = remoteExecutionConfiguration() )
    {
        rd.reset(new insight::MountRemote(*rec));
        exePath = rd->mountpoint();
    }

    {
      std::ofstream f( (exePath/"wnow").c_str() );
      f.close();
    }
  }
}




void AnalysisForm::onWnowAndStop()
{
  if (isRunning())
  {
    fs::path exePath = localCaseDirectory();
    std::unique_ptr<insight::MountRemote> rd;
    if ( auto* rec = remoteExecutionConfiguration() )
    {
        rd.reset(new insight::MountRemote(*rec));
        exePath = rd->mountpoint();
    }

    {
      std::ofstream f( (exePath/"wnowandstop").c_str() );
      f.close();
    }
  }
}




void AnalysisForm::onShell()
{
  auto locDir = QString::fromStdString(localCaseDirectory().string());

  if ( auto* rec = remoteExecutionConfiguration() )
  {
    if (rec->remoteDirExists())
    {
      QStringList args;
      if ( !QProcess::startDetached("isRemoteShell.sh",
                                    args, locDir)
           )
      {
        QMessageBox::critical(
              this,
              "Failed to start",
              "Failed to start remote shell in directoy "+locDir
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
            this,
            "Failed to start",
            "Failed to start mate-terminal in directoy "+locDir
            );
    }
  }
}

