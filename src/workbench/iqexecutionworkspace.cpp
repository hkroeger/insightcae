
#include "iqexecutionworkspace.h"
#include "base/tools.h"
#include "qexecutionworkspacedialog.h"

#include <QMessageBox>

#include "analysisform.h"
#include "base/translations.h"


IQRemoteExecutionState* IQExecutionWorkspace::defaultOpenFOAMRemoteWorkspace()
{
#ifdef WSL_DEFAULT
    return IQRemoteExecutionState::New<IQWorkbenchRemoteExecutionState>(
                af_,
                insight::remoteServers.findFirstServerOfType<insight::WSLLinuxServer>(".*")
                );
//#else
//    // for test purposes only
//    return IQRemoteExecutionState::New<IQWorkbenchRemoteExecutionState>(
//                af_,
//                insight::remoteServers.findServer("localhost")
//                );
#endif
    return nullptr;
}


void IQExecutionWorkspace::connectReinitializeToDefault()
{
    QObject::connect(
               remoteExecutionConfiguration_, &QObject::destroyed, remoteExecutionConfiguration_,
                [this](QObject *obj)
                {
                    insight::dbg()<<"resetting to defaults"<<std::endl;
                    initializeToDefaults();
                }
    );
}



IQExecutionWorkspace::IQExecutionWorkspace(AnalysisForm *af)
    : af_(af)
{}



void IQExecutionWorkspace::initializeToDefaults()
{
    if (//af_->isOpenFOAMAnalysis()
        (af_->compatibleOperatingSystems().count(
             insight::currentOperatingSystem) < 1)
            && !remoteExecutionConfiguration_  )
    {

        if ( (remoteExecutionConfiguration_ = defaultOpenFOAMRemoteWorkspace()) != nullptr )
        {
            if ( af_->remoteExecutionConfiguration_->remoteHostRunningAndDirectoryExisting()
                 && !af_->localCaseDirectory_->empty() )
            {
                af_->remoteExecutionConfiguration_->commit( af_->localCaseDirectory() );
            }

            connectReinitializeToDefault();
        }
    }
}




bool IQExecutionWorkspace::hasLocalWorkspace() const
{
    return localCaseDirectory_!=nullptr;
}




bool IQExecutionWorkspace::localWorkspaceIsTemporary() const
{
    if (
      localCaseDirectory_
      &&
      localCaseDirectory_->isPersistent()
    ) return false;

    return true;
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
                        af_, false, "analysis" ) );
    }
    return *localCaseDirectory_;
}




IQRemoteExecutionState* IQExecutionWorkspace::remoteExecutionConfiguration()
{
  return remoteExecutionConfiguration_;
}




void IQExecutionWorkspace::resetExecutionEnvironment(
        const boost::filesystem::path& lwd,
        boost::variant<insight::RemoteLocation*,boost::blank> newRemoteLocation )
{
    bool lwd_changed=false;
    if (
            !localCaseDirectory_ // not yet set
            ||
            (boost::filesystem::canonical(*localCaseDirectory_)
                != boost::filesystem::canonical(lwd)) // path changed
            ||
            ( lwd.empty() && localCaseDirectory_->isPersistent() ) // exchange dir for temp dir
         )
    {
        insight::dbg()
                << "changing local workspace from " << (localCaseDirectory_?(localCaseDirectory_->string()):"(unset)")
                << " to "
                << lwd << std::endl;

        localCaseDirectory_.reset(); // delete old one FIRST

        insight::dbg()<<"new local directory set: "<<lwd<<std::endl;
        if (!lwd.empty())
        {
            localCaseDirectory_.reset(
                        new IQCaseDirectoryState(
                            af_, lwd ) );
        }
        lwd_changed=true;
    }

    // check for present remote config in new WD
    std::unique_ptr<insight::RemoteLocation> presentRL;
    if ( lwd_changed
         && localCaseDirectory_
         && localCaseDirectory_->isPersistent() )
    {
        insight::dbg()<<"checking for present remote config in "<<*localCaseDirectory_<<std::endl;
        if (insight::RemoteExecutionConfig::remoteLocationConfigIsValid(
                insight::RemoteExecutionConfig::defaultConfigFile(lwd)) )
        {
            presentRL = std::make_unique<insight::RemoteLocation>(
                        insight::RemoteExecutionConfig::defaultConfigFile(lwd) );

            insight::dbg()<<"remote location configuration read from directory "<<lwd<<std::endl;
        }
        else
        {
            insight::dbg()<<"no remote location configuration in directory "<<lwd<<std::endl;
        }
    }

    // if there is REC present in new WD and no RWD specified, use the present one
    if ( (newRemoteLocation.type() == typeid(boost::blank)) && presentRL )
    {
        insight::dbg()<<"appling read remote cfg"<<std::endl;
        newRemoteLocation = presentRL.get();
    }


    if ( newRemoteLocation.type() == typeid(insight::RemoteLocation*) )
    {
        if (remoteExecutionConfiguration_)
        {
            insight::dbg()<<"deleting old remote config"<<std::endl;
            QObject::disconnect(remoteExecutionConfiguration_, &QObject::destroyed, 0, 0);
            delete remoteExecutionConfiguration_;
        }

        if (auto *nrl = boost::get<insight::RemoteLocation*>(newRemoteLocation))
        {

            insight::dbg()<<"setting new remote config"<<std::endl;
            remoteExecutionConfiguration_ =
                    IQRemoteExecutionState::New<IQWorkbenchRemoteExecutionState>(
                        af_,
                        *nrl );

            // if the remote location is ready available, commit it immediately
            if (remoteExecutionConfiguration_->remoteHostRunningAndDirectoryExisting())
                remoteExecutionConfiguration_->commit( localCaseDirectory() );

            connectReinitializeToDefault();
        }
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

    if (af_->compatibleOperatingSystems().count(
            insight::currentOperatingSystem) < 1)
    {
        dlg.lockRemoteExecution(
            _("Please note: the local execution of the selected analysis is not possible on the current operating system."
            "\n"
              " Therefore the execution is locked to a WSL backend or a remote host.") );
    }


    if (dlg.exec() == QDialog::Accepted)
    {
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
             .arg(QString::fromStdString( remoteExecutionConfiguration_->location().remoteDir().string()))
             .arg(QString::fromStdString( remoteExecutionConfiguration_->location().serverConfig() ))
            );
      if (answer==QMessageBox::Yes)
      {
        remoteExecutionConfiguration_->cleanup(true);
        delete remoteExecutionConfiguration_;
      }
    }
}




void IQExecutionWorkspace::prepareDeletion()
{
    if (remoteExecutionConfiguration_)
    {
        QObject::disconnect(
                remoteExecutionConfiguration_, &QObject::destroyed,
                0, 0);
    }
}
