#ifndef IQEXECUTIONENVIRONMENT_H
#define IQEXECUTIONENVIRONMENT_H

#include "boost/none.hpp"

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

    IQRemoteExecutionState* defaultOpenFOAMRemoteWorkspace();
    void connectReinitializeToDefault();

public:
    IQExecutionWorkspace(AnalysisForm *af);

    void initializeToDefaults();

    // access functions
    bool hasLocalWorkspace() const;
    bool localWorkspaceIsTemporary() const;
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
            boost::variant<insight::RemoteLocation*,boost::blank> remoteLocation = boost::blank()
            );

    void showSetupExecutionEnvironmentDialog();

    void removeRemoteWorkspace();

    void prepareDeletion();
};

#endif // IQEXECUTIONENVIRONMENT_H
