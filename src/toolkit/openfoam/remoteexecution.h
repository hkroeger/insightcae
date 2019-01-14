#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"

namespace insight
{


struct RemoteServerInfo
{
    std::string serverName_;
    bfs_path defaultDir_;
};


class RemoteServerList
    : public std::map<std::string, RemoteServerInfo>
{
public:
  RemoteServerList();
};


extern RemoteServerList remoteServers;


class RemoteExecutionConfig
{
protected:
    bfs_path meta_file_;
    std::string server_;
    boost::filesystem::path localDir_, remoteDir_;

    void execRemoteCmd(const std::string& cmd);

public:
    RemoteExecutionConfig(const boost::filesystem::path& location, bool needConfig=true, const bfs_path& meta_file="");

    const std::string& server() const;
    const boost::filesystem::path& localDir() const;
    const boost::filesystem::path& remoteDir() const;

    std::vector<bfs_path> remoteLS() const;

    void syncToRemote();
    void syncToLocal(bool skipTimeSteps=false);

    void queueRemoteCommand(const std::string& command, bool waitForPreviousFinished=true);
    void waitRemoteQueueFinished();
    void waitLastCommandFinished();

    void cancelRemoteCommands();
    void removeRemoteDir();

    bool isValid() const;
};

}

#endif // REMOTEEXECUTION_H
