
#include "iqremoteexecutionstate.h"
#include "base/exception.h"



void IQRemoteExecutionState::updateGUI(bool /*enabled*/)
{}



IQRemoteExecutionState::IQRemoteExecutionState()
  : QObject(nullptr)
{}


bool IQRemoteExecutionState::remoteHostRunningAndDirectoryExisting() const
{
    return rlc_->isActive();
}

void IQRemoteExecutionState::commit(const boost::filesystem::path &location)
{
  if (!dynamic_cast<const insight::RemoteExecutionConfig*>(rlc_.get()))
  {
    auto rec = std::make_unique<insight::RemoteExecutionConfig>(location, *rlc_);
    rec->initialize();
    rec->writeConfig(); // re-write with temporary directory resolved.
    rlc_ = std::move(rec);
    updateGUI(true);
  }
}

bool IQRemoteExecutionState::isCommitted() const
{
  return dynamic_cast<insight::RemoteExecutionConfig*>(rlc_.get());
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



