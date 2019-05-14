#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"
#include "boost/process.hpp"

namespace insight
{


class TaskSpoolerInterface
{
  std::string remote_machine_;
  boost::filesystem::path socket_;
  boost::process::environment env_;
  boost::asio::io_service ios_;
  boost::process::async_pipe tail_cout_;

  std::thread ios_run_thread_;

  std::shared_ptr<boost::process::child> tail_c_;
  std::vector< std::function<void(std::string)> > receivers_;

//  static const int max_read_length = 256; // maximum amount of data to read in one operation
//  char read_msg_[max_read_length]; // data read from the socket
  boost::asio::streambuf buf_cout_;

  void read_start(void);
  void read_complete(const boost::system::error_code& error, size_t bytes_transferred);

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
  TaskSpoolerInterface(const boost::filesystem::path& socket, const std::string& remote_machine="");
  ~TaskSpoolerInterface();

  JobList jobs() const;

  int clean();
  int kill();

  void startTail(std::function<void(std::string)> receiver);
  void stopTail();

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

    boost::filesystem::path socket() const;

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
