#ifndef TASKSPOOLERINTERFACE_H
#define TASKSPOOLERINTERFACE_H

#include "base/boost_include.h"
#include "base/remoteserverlist.h"
#include "boost/process.hpp"
#include "boost/asio/io_service.hpp"


namespace insight
{


class TaskSpoolerInterface
{
  RemoteServerPtr server_;
  boost::filesystem::path socket_;
  boost::process::environment env_;

  std::shared_ptr<boost::asio::io_service> ios_;
  std::shared_ptr<boost::process::async_pipe> tail_cout_;
  std::shared_ptr<std::thread> ios_run_thread_;
  std::shared_ptr<boost::process::child> tail_c_;
  std::shared_ptr<boost::asio::streambuf> buf_cout_;

  std::vector< std::function<void(const std::string&)> > receivers_;

//  static const int max_read_length = 256; // maximum amount of data to read in one operation
//  char read_msg_[max_read_length]; // data read from the socket

  void read_start(void);
  void read_complete(const boost::system::error_code& error, size_t bytes_transferred);

public:
  enum JobState { Running, Queued, Finished, Unknown };

  struct Job
  {
    int id;
    JobState state;
    boost::filesystem::path output;
    int elevel;
    std::string commandLine;
  };

  struct JobList
  : public std::vector<Job>
  {
    bool hasRunningJobs() const;
    bool hasQueuedJobs() const;
    bool hasFailedJobs() const;
  };

public:
  TaskSpoolerInterface(const boost::filesystem::path& socket, RemoteServerPtr server = RemoteServerPtr() );
  ~TaskSpoolerInterface();

  JobList jobs() const;

  int clean();
  int kill();

  // =============================
  // tail
  void startTail(std::function<void(const std::string&)> receiver, bool blocking=false);
  bool isTailRunning() const;
  void stopTail();

  // =============================
  // jobs
  int startJob(const std::vector<std::string>& commandline);
  void cancelAllJobs();

  int stopTaskspoolerServer();
};


}

#endif // TASKSPOOLERINTERFACE_H
