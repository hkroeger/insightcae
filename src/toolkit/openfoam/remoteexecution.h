#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"

namespace insight
{

class RemoteExecutionConfig
{
protected:
    bfs_path meta_file_;
    std::string server_;
    boost::filesystem::path localDir_, remoteDir_;

    void execRemoteCmd(const std::string& cmd);
    bool isValid() const;

public:
    RemoteExecutionConfig(const boost::filesystem::path& location, bool needConfig=true, const bfs_path& meta_file="");

    const std::string& server() const;
    const boost::filesystem::path& localDir() const;
    const boost::filesystem::path& remoteDir() const;

    void syncToRemote();
    void syncToLocal();

    void queueRemoteCommand(const std::string& command, bool waitForPreviousFinished=true);
    void waitRemoteQueueFinished();
    void waitLastCommandFinished();

    void cancelRemoteCommands();
    void removeRemoteDir();
};

}

#endif // REMOTEEXECUTION_H
