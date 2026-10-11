#include "iqworkbenchremoteexecutionstate.h"

#include "analysisform.h"
#include "ui_analysisform.h"

#include "base/translations.h"



void IQWorkbenchRemoteExecutionState::toggleRemoteGUIElements(bool remoteEnabled)
{
    if (auto af = dynamic_cast<AnalysisForm*>(parent()))
    {
        auto* ui = af->ui;

        if (remoteEnabled)
        {
            auto *m = new QMenu(af->ui->btnParaview);
            auto *a = new QAction(_("launch in local location"));
            m->addAction(a);
            connect(a, &QAction::triggered, af, &AnalysisForm::onStartPVLocal );

            a=new QAction(_("launch in remote workspace"));
            m->addAction(a);
            connect(a, &QAction::triggered, af, &AnalysisForm::onStartPVRemote );

            ui->btnParaview->setMenu(m);
        }
        else
        {
            if (auto *m = ui->btnParaview->menu())
            {
                ui->btnParaview->setMenu(nullptr);
                m->deleteLater();
            }
        }
    }
}




void IQWorkbenchRemoteExecutionState::updateGUI(bool enabled)
{
  insight::dbg()<<"set IQWorkbenchRemoteExecutionState to enabled="
                <<enabled<<std::endl;

  if (auto af = dynamic_cast<AnalysisForm*>(parent()))
  {
      auto* ui = af->ui;

      ui->lblHostName->setEnabled(enabled);
      if (enabled)
      {
        if (rlc_->isActive())
        {
          ui->lblHostName->setText(
                QString::fromStdString( rlc_->serverLabel() ) );
          ui->lblHostName->setToolTip(QString());
        }
        else
        {
          ui->lblHostName->setText(
                QString(_("%1 (not connected)")).arg(
                  QString::fromStdString( rlc_->serverLabel() ) ) );
          ui->lblHostName->setToolTip(
                _("The remote server is currently not reachable or not running.\n"
                  "A connection will be established (and the server launched, if possible)"
                  " when a remote action is started.") );
        }
      }
      else
      {
        ui->lblHostName->setText(_("(none)"));
        ui->lblHostName->setToolTip(QString());
      }

      ui->lblRemoteDirectory->setEnabled(enabled);
      if (enabled)
      {
          if (rlc_->remoteDir().empty())
          {
            ui->lblRemoteDirectory->setText(_("temporary directory"));
          }
          else
          {
            ui->lblRemoteDirectory->setText(
                QString(_("directory %1")).arg(
                    QString::fromStdString( rlc_->remoteDir().string() ) ) );
          }
      }
      else
      {
          ui->lblRemoteDirectory->setText(_("(none)"));
      }

      if (dynamic_cast<const insight::WSLLinuxServer::Config*>(&rlc_->serverConfig()))
      {
        ui->btnDisconnect->setEnabled(false);
        ui->btnResume->setEnabled(false);
      }
      else
      {
        ui->btnDisconnect->setEnabled(enabled);
        ui->btnResume->setEnabled(enabled);
      }
      ui->btnUpload->setEnabled(enabled);
      ui->btnDownload->setEnabled(enabled);
      ui->btnRemoveRemote->setEnabled(enabled);
      ui->lblRemote_1->setEnabled(enabled);
      ui->lblRemote_2->setEnabled(enabled);

      if ( enabled )
      {
          if (af->localWorkspaceIsTemporary() )
          {
              ui->cbDownloadWhenFinished->setChecked(false);
          }
          else
          {
              ui->cbDownloadWhenFinished->setChecked(true);
          }
      }
      ui->cbDownloadWhenFinished->setEnabled(enabled);
  }

  insight::dbg()<<"IQWorkbenchRemoteExecutionState updated"<<std::endl;
}




void IQWorkbenchRemoteExecutionState::commit(const boost::filesystem::path &location)
{
    IQRemoteExecutionState::commit(location);
    toggleRemoteGUIElements(true);
}



void IQWorkbenchRemoteExecutionState::cleanup(bool forceRemoval)
{
    IQRemoteExecutionState::cleanup(forceRemoval); // calls discard()
}



void IQWorkbenchRemoteExecutionState::discard()
{
    toggleRemoteGUIElements(false);
    IQRemoteExecutionState::discard();
}

