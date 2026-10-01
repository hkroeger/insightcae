#ifndef INSIGHT_LINUXREMOTESERVER_H
#define INSIGHT_LINUXREMOTESERVER_H

#include "remoteserver.h"
#include "boost/process/detail/child_decl.hpp"
#include "boost/process/pipe.hpp"
#include <functional>
#include <memory>

namespace insight {

std::string toUnixPath(const boost::filesystem::path& wp);

class LinuxRemoteServer
    : public RemoteServer
{
public:
    struct Config : public RemoteServer::Config
    {
        Config(const boost::filesystem::path& bp, int np);
        bool isRunning() const override;
        int occupiedProcessors(int* nProcAvail=nullptr) const override;
    };

#ifndef SWIG
    struct SSHRemoteStream : public RemoteStream {
        std::unique_ptr<boost::process::child> child_;
        boost::process::opstream s_;
        boost::filesystem::path remoteFilePath_;
        bool closed_ = false;

        ~SSHRemoteStream();
        std::ostream& stream() override;
        void close() override;
    };

    std::unique_ptr<RemoteStream> remoteOFStream
        (
            const boost::filesystem::path& remoteFilePath,
            int totalBytes,
            std::function<void(int progress,const std::string& status_text)> progress_callback =
            std::function<void(int,const std::string&)>()
            ) override;
#endif

  /**
   * @brief The ProcessGroupJob struct
   * background job, which runs in its own session / process group on the server.
   * kill() terminates the whole group, including child processes (e.g. solvers).
   */
  struct ProcessGroupJob : public RemoteServer::BackgroundJob
  {
  protected:
    int pgid_;
  public:
    ProcessGroupJob(RemoteServer& server, int pgid);
    void kill() override;
    bool isRunning() override;
    inline int pgid() const { return pgid_; }
  };

  /**
   * @brief launchBackgroundProcess
   * start cmd in a new session (setsid). The process group id is reported back
   * by the job itself and used for kill().
   * If expectedOutputBeforeDetach is empty, the output of cmd is discarded
   * and the job does not depend on the connection to the server.
   * Otherwise, the output is read until the expected patterns are found
   * and remains connected to the (then no longer read) channel:
   * such jobs should not produce much output afterwards and may end with the connection.
   */
  BackgroundJobPtr launchBackgroundProcess(
      const std::string& cmd,
      const std::vector<ExpectedOutput>& expectedOutputBeforeDetach = {} ) override;

  bool checkIfDirectoryExists(const boost::filesystem::path& dir) override;
  boost::filesystem::path getTemporaryDirectoryName(const boost::filesystem::path& templatePath) override;
  void createDirectory(const boost::filesystem::path& remoteDirectory) override;
  void removeDirectory(const boost::filesystem::path& remoteDirectory) override;
  std::vector<boost::filesystem::path> listRemoteDirectory(const boost::filesystem::path& remoteDirectory) override;
  std::vector<boost::filesystem::path> listRemoteSubdirectories(const boost::filesystem::path& remoteDirectory) override;

  int findFreeRemotePort() const override;
};

} // namespace insight

#endif // INSIGHT_LINUXREMOTESERVER_H
