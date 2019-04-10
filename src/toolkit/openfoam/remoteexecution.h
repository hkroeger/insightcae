#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"
#include "boost/process.hpp"

namespace insight
{


class TaskSpoolerInterface
{
  boost::filesystem::path socket_;
  boost::process::environment env_;

public:
  enum JobState { Running, Queued, Finished, Unknown };

  struct Job
  {
    int id;
    JobState state;
    boost::filesystem::path output;
    std::string remainder;
  };

  struct JobList
  : public std::vector<Job>
  {
    bool hasRunningJobs() const;
    bool hasQueuedJobs() const;
  };

public:
  TaskSpoolerInterface(const boost::filesystem::path& socket);

  JobList jobs() const;

  void cancelAllJobs();
};


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

#ifndef SWIG
  const RemoteServerList::value_type findServer(const std::string& server) const;
#endif
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

    const boost::filesystem::path& metaFile() const;

    std::vector<bfs_path> remoteLS() const;

    void syncToRemote(const std::vector<std::string>& exclude_pattern = std::vector<std::string>() );
    void syncToLocal(bool skipTimeSteps=false, const std::vector<std::string>& exclude_pattern = std::vector<std::string>() );

    void queueRemoteCommand(const std::string& command, bool waitForPreviousFinished=true);
    void waitRemoteQueueFinished();
    void waitLastCommandFinished();

    void cancelRemoteCommands();
    void removeRemoteDir();

    bool isValid() const;
};

}

#endif // REMOTEEXECUTION_H
