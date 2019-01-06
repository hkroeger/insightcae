#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"

namespace insight
{

class RemoteExecutionConfig
{
protected:
    std::string server_;
    boost::filesystem::path localDir_, remoteDir_;

    void execRemoteCmd(const std::string& cmd);

public:
    RemoteExecutionConfig(const boost::filesystem::path& location, bool needConfig=true);

    const std::string& server() const;
    const boost::filesystem::path& localDir() const;
    const boost::filesystem::path& remoteDir() const;

    void syncToRemote();
    void syncToLocal();

    void queueRemoteCommand(const std::string& command);
    void waitRemoteQueueFinished();

    void cancelRemoteCommands();
    void removeRemoteDir();
};

}

#endif // REMOTEEXECUTION_H
