#ifndef IQWORKBENCHREMOTEEXECUTIONSTATE_H
#define IQWORKBENCHREMOTEEXECUTIONSTATE_H

#include "iqremoteexecutionstate.h"

class IQWorkbenchRemoteExecutionState
    : public IQRemoteExecutionState
{
    void toggleRemoteGUIElements(bool remoteEnabled);

protected:
    void updateGUI(bool enabled) override;
    void commit(const boost::filesystem::path& location) override;
    void cleanup(bool forceRemoval=false) override;
};

#endif // IQWORKBENCHREMOTEEXECUTIONSTATE_H
