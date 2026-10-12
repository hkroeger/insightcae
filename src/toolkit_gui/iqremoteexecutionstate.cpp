
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
    // initialize a copy first: if it fails, the state and the
    // configuration file remain untouched
    insight::RemoteLocation rl(*rlc_);
    rl.initialize(true);
    // writes the config file (with temporary directory resolved)
    rlc_ = std::make_unique<insight::RemoteExecutionConfig>(location, rl);
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
  discard();
}




void IQRemoteExecutionState::serverStopped()
{
  // keep only the location part (uncommit)
  auto rl = std::make_unique<insight::RemoteLocation>(*rlc_);
  rl->markInactive();
  rlc_ = std::move(rl);
  updateGUI(true);
}




void IQRemoteExecutionState::discard()
{
  deleteLater();
}



