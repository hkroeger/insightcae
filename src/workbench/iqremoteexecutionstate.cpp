
#include "iqremoteexecutionstate.h"
#include "base/exception.h"

#include "analysisform.h"
#include "ui_analysisform.h"

#include "base/wsllinuxserver.h"

void IQRemoteExecutionState::setAFEnabledState(bool enabled)
{
  insight::dbg()<<"set IQRemoteExecutionState to enabled="
                <<enabled<<std::endl;

  af_->ui->lblHostName->setEnabled(enabled);
  if (enabled)
    af_->ui->lblHostName->setText(
          QString::fromStdString( rlc_->serverLabel() ) );
  else
    af_->ui->lblHostName->setText("(none)");

  af_->ui->lblRemoteDirectory->setEnabled(enabled);
  if (enabled)
    af_->ui->lblRemoteDirectory->setText(
          QString::fromStdString( rlc_->remoteDir().string() ) );
  else
    af_->ui->lblRemoteDirectory->setText("(none)");

  if (std::dynamic_pointer_cast<insight::WSLLinuxServer::Config>(rlc_->serverConfig()))
  {
    af_->ui->btnDisconnect->setEnabled(false);
    af_->ui->btnResume->setEnabled(false);
  }
  else
  {
    af_->ui->btnDisconnect->setEnabled(enabled);
    af_->ui->btnResume->setEnabled(enabled);
  }
  af_->ui->btnUpload->setEnabled(enabled);
  af_->ui->btnDownload->setEnabled(enabled);
  af_->ui->btnRemoveRemote->setEnabled(enabled);
  af_->ui->lblRemote_1->setEnabled(enabled);
  af_->ui->lblRemote_2->setEnabled(enabled);

  insight::dbg()<<"IQRemoteExecutionState updated"<<std::endl;
}




IQRemoteExecutionState::IQRemoteExecutionState(
    AnalysisForm *af,
    const insight::RemoteLocation &remoteLocation )
  : QObject(af),
    af_(af),
    rlc_(new insight::RemoteLocation(remoteLocation))
{
  setAFEnabledState(true);
}




IQRemoteExecutionState::IQRemoteExecutionState(
    AnalysisForm *af,
    const boost::filesystem::path& configFile
    )
  : QObject(af),
    af_(af),
    rlc_(new insight::RemoteLocation(configFile))
{
  setAFEnabledState(true);
}




IQRemoteExecutionState::IQRemoteExecutionState(
    AnalysisForm *af,
    insight::RemoteServer::ConfigPtr rsc,
    const boost::filesystem::path& remotePath
    )
  : QObject(af),
    af_(af),
    rlc_(new insight::RemoteLocation(rsc, remotePath))
{
  setAFEnabledState(true);
}




IQRemoteExecutionState::~IQRemoteExecutionState()
{
  setAFEnabledState(false);
}




void IQRemoteExecutionState::commit(const boost::filesystem::path &location)
{
  if (!dynamic_cast<const insight::RemoteExecutionConfig*>(rlc_.get()))
  {
    auto rl = std::move(rlc_);
    rlc_.reset(
        new insight::RemoteExecutionConfig(location, *rl)
        );
    rlc_->initialize();

    setAFEnabledState(true);
  }
}




insight::RemoteLocation &IQRemoteExecutionState::location() const
{
  return *rlc_;
}




insight::RemoteExecutionConfig& IQRemoteExecutionState::exeConfig()
{
  insight::RemoteExecutionConfig* rec =
      dynamic_cast<insight::RemoteExecutionConfig*>(rlc_.get());

  insight::assertion(
        bool(rec),
        "internal error: remote location has not been committed before usage" );

  return *rec;
}




void IQRemoteExecutionState::cleanup(bool forceRemoval)
{
  rlc_->cleanup(forceRemoval);
  deleteLater();
}



