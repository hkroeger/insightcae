
#include "iqremoteexecutionstate.h"
#include "base/exception.h"



void IQRemoteExecutionState::updateGUI(bool /*enabled*/)
{}



IQRemoteExecutionState::IQRemoteExecutionState( /*QObject* p*/ )
  : QObject(/*p*/nullptr)
{
}



//IQRemoteExecutionState* IQRemoteExecutionState::New(
//    QObject *af,
//    const insight::RemoteLocation &remoteLocation )
//{
//  auto *iqr = new IQRemoteExecutionState(af);
//  iqr->rlc_.reset(new insight::RemoteLocation(remoteLocation));
//  iqr->updateGUI(true);
//  return iqr;
//}




//IQRemoteExecutionState* IQRemoteExecutionState::New(
//    QObject *af,
//    const boost::filesystem::path& configFile
//    )
//{
//  auto *iqr = new IQRemoteExecutionState(af);
//  iqr->rlc_.reset(new insight::RemoteLocation(configFile));
//  iqr->updateGUI(true);
//  return iqr;
//}




//IQRemoteExecutionState* IQRemoteExecutionState::New(
//    QObject *af,
//    insight::RemoteServer::ConfigPtr rsc,
//    const boost::filesystem::path& remotePath
//    )
//{
//  auto *iqr = new IQRemoteExecutionState(af);
//  iqr->rlc_.reset(new insight::RemoteLocation(rsc, remotePath));
//  iqr->updateGUI(true);
//  return iqr;
//}




//IQRemoteExecutionState::~IQRemoteExecutionState()
//{
//  updateGUI(false);
//}




void IQRemoteExecutionState::commit(const boost::filesystem::path &location)
{
  if (!dynamic_cast<const insight::RemoteExecutionConfig*>(rlc_.get()))
  {
    auto rl = std::move(rlc_);
    rlc_.reset(
        new insight::RemoteExecutionConfig(location, *rl)
        );
    rlc_->initialize();

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



