
#include "analysisform.h"
#include "ui_analysisform.h"

#include "iqexecutionworkspace.h"
#include "qexecutionworkspacedialog.h"

#include <QMessageBox>



void IQWorkbenchRemoteExecutionState::updateGUI(bool enabled)
{
  insight::dbg()<<"set IQWorkbenchRemoteExecutionState to enabled="
                <<enabled<<std::endl;

  if (auto af=dynamic_cast<AnalysisForm*>(parent()))
  {
      auto* ui = af->ui;

      ui->lblHostName->setEnabled(enabled);
      if (enabled)
        ui->lblHostName->setText(
              QString::fromStdString( rlc_->serverLabel() ) );
      else
        ui->lblHostName->setText("(none)");

      ui->lblRemoteDirectory->setEnabled(enabled);
      if (enabled)
      {
          if (rlc_->remoteDir().empty())
          {
              ui->lblRemoteDirectory->setText("temporary directory");
          }
          else
          {
              ui->lblRemoteDirectory->setText( "directory "+
                  QString::fromStdString( rlc_->remoteDir().string() ) );
          }
      }
      else
      {
          ui->lblRemoteDirectory->setText("(none)");
      }

      if (std::dynamic_pointer_cast<insight::WSLLinuxServer::Config>(rlc_->serverConfig()))
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
  }

  insight::dbg()<<"IQWorkbenchRemoteExecutionState updated"<<std::endl;
}




IQExecutionWorkspace::IQExecutionWorkspace(AnalysisForm *af)
    : af_(af)
{}


bool IQExecutionWorkspace::hasLocalWorkspace() const
{
    return localCaseDirectory_!=nullptr;
}


bool IQExecutionWorkspace::hasRemoteWorkspace() const
{
    return remoteExecutionConfiguration_!=nullptr;
}

bool IQExecutionWorkspace::hasActiveRemoteWorkspace() const
{
    return hasRemoteWorkspace() && remoteExecutionConfiguration_->isCommitted();
}


boost::filesystem::path IQExecutionWorkspace::localCaseDirectory() const
{
    if (!localCaseDirectory_)
    {
        const_cast<std::unique_ptr<IQCaseDirectoryState>&>
                (localCaseDirectory_).reset(
                    new IQCaseDirectoryState(
                        af_, false ) );
    }
    return *localCaseDirectory_;
}




IQRemoteExecutionState* IQExecutionWorkspace::remoteExecutionConfiguration()
{
#ifdef WSL_DEFAULT
  if (af_->isOpenFOAMAnalysis()
          && !remoteExecutionConfiguration_ && !remoteExeConfigWasEdited_ )
  {
    af_->remoteExecutionConfiguration_ =
        IQRemoteExecutionState::New<IQWorkbenchRemoteExecutionState>(
          af_,
          insight::remoteServers.findFirstServerOfType<insight::WSLLinuxServer>(".*")
          );
    if ( af_->remoteExecutionConfiguration_->remoteHostRunningAndDirectoryExisting()
            && !af_->localCaseDirectory_->empty() )
        af_->remoteExecutionConfiguration_->commit( af_->localCaseDirectory() );

  }
#endif
  return remoteExecutionConfiguration_;
}




void IQExecutionWorkspace::resetExecutionEnvironment(
        const boost::filesystem::path& lwd,
        insight::RemoteLocation* newRemoteLocation )
{
    bool lwd_changed=false;
    if ( !localCaseDirectory_ ||
         (boost::filesystem::canonical(*localCaseDirectory_)
          != boost::filesystem::canonical(lwd))
         )
    {
        localCaseDirectory_.reset(); // delete old one FIRST

        insight::dbg()<<"new local directory set: "<<lwd<<std::endl;
        if (lwd.empty())
        {
            localCaseDirectory_.reset(
                        new IQCaseDirectoryState(
                            af_, false, "analysis" ) );
        }
        else
        {
            localCaseDirectory_.reset(
                        new IQCaseDirectoryState(
                            af_, lwd ) );
        }

        lwd_changed=true;
    }

    // check for present remote config in new WD
    std::unique_ptr<insight::RemoteLocation> presentRL;
    if ( lwd_changed && localCaseDirectory_->keep() )
    {
        try
        {
            presentRL = std::make_unique<insight::RemoteLocation>(
                        insight::RemoteExecutionConfig::defaultConfigFile(lwd) );

            insight::dbg()<<"remote location configuration read from directory "<<lwd<<std::endl;
        }
        catch (...)
        {
            insight::dbg()<<"no remote location configuration in directory "<<lwd<<std::endl;
        }
    }

    // if there is REC present in new WD and no RWD specified, use the present one
    if ( !newRemoteLocation && presentRL )
    {
        newRemoteLocation = presentRL.get();
    }


    if (newRemoteLocation)
    {
        if (remoteExecutionConfiguration_)
            delete remoteExecutionConfiguration_;


        insight::assertion(
                    localCaseDirectory_->keep(),
                    "internal error: "
                    "remote execution with temporary local working directory is not allow!" );

        remoteExecutionConfiguration_ =
                IQRemoteExecutionState::New<IQWorkbenchRemoteExecutionState>(
                    af_,
                    *newRemoteLocation );

        // if the remote location is available, commit it already
        if ( remoteExecutionConfiguration_->remoteHostRunningAndDirectoryExisting() )
            remoteExecutionConfiguration_->commit( localCaseDirectory() );
    }
}



void IQExecutionWorkspace::showSetupExecutionEnvironmentDialog()
{
    insight::RemoteLocation *rl = nullptr;
    if (auto* rec = remoteExecutionConfiguration())
      rl=&rec->location();

    QExecutionWorkspaceDialog dlg(
          localCaseDirectory_.get(),
          rl,
          af_ );

    if (dlg.exec() == QDialog::Accepted)
    {
      remoteExeConfigWasEdited_ = true;

      resetExecutionEnvironment(
                  dlg.localDirectory(),
                  dlg.remoteLocation()
                  );
    }
}




void IQExecutionWorkspace::removeRemoteWorkspace()
{
    if (remoteExecutionConfiguration_)
    {
      auto answer = QMessageBox::question(
            af_, "Decision required",
            QString("The directory %1 on server %2 and its contents will be deleted!\nContinue?")
             .arg(QString::fromStdString(remoteExecutionConfiguration_->location().remoteDir().string()))
             .arg(QString::fromStdString( *(remoteExecutionConfiguration_->location().serverConfig()) ))
            );
      if (answer==QMessageBox::Yes)
      {
        remoteExecutionConfiguration_->cleanup(true);
        delete remoteExecutionConfiguration_;
      }
   }
}
