#ifndef IQEXECUTIONENVIRONMENT_H
#define IQEXECUTIONENVIRONMENT_H

#include <QPointer>

#include "iqcasedirectorystate.h"
#include "iqremoteexecutionstate.h"

class AnalysisForm;
class WorkbenchAction;



class IQWorkbenchRemoteExecutionState
    : public IQRemoteExecutionState
{
protected:
  void updateGUI(bool enabled) override;
};



class IQExecutionWorkspace
{
    AnalysisForm *af_;

    std::unique_ptr<IQCaseDirectoryState> localCaseDirectory_;
    QPointer<IQRemoteExecutionState> remoteExecutionConfiguration_;
    bool remoteExeConfigWasEdited_ = false;

public:
    IQExecutionWorkspace(AnalysisForm *af);

    // access functions

    bool hasLocalWorkspace() const;
    bool hasRemoteWorkspace() const;
    bool hasActiveRemoteWorkspace() const;

    /**
     * @brief localCaseDirectory
     * Returns selected local working direcory. Creates the directory, if required.
     * @return local case directory path
     */
    boost::filesystem::path localCaseDirectory() const;

    /**
     * @brief remoteExecutionConfiguration
     *
     * @return
     * reference to remote config
     */
    IQRemoteExecutionState* remoteExecutionConfiguration();

    /**
     * @brief resetLocalCaseDirectory
     * change working directory.
     * This will reset remote exe config also.
     * @param localDirectory
     * @param remoteLocation
     */
    void resetExecutionEnvironment(
            const boost::filesystem::path& localDirectory,
            insight::RemoteLocation* remoteLocation = nullptr
            );

    void showSetupExecutionEnvironmentDialog();

    void removeRemoteWorkspace();
};

#endif // IQEXECUTIONENVIRONMENT_H
