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
        else if (isCommitted())
        {
          ui->lblHostName->setText(
                QString(_("%1 (remote directory not found)")).arg(
                  QString::fromStdString( rlc_->serverLabel() ) ) );
          ui->lblHostName->setToolTip(
                _("The server is reachable, but the remote directory does not exist.") );
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

      // pass this explicitly: when disabling, this state is about to be destroyed
      af->updateRemoteActionAvailability( enabled ? this : nullptr );
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
      // enabling: see AnalysisForm::updateRemoteActionAvailability
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



void IQWorkbenchRemoteExecutionState::serverStopped()
{
    toggleRemoteGUIElements(false); // e.g. remote paraview is not possible anymore
    IQRemoteExecutionState::serverStopped();
}



void IQWorkbenchRemoteExecutionState::discard()
{
    toggleRemoteGUIElements(false);
    IQRemoteExecutionState::discard();
}

