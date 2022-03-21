#ifndef INSIGHT_EXTERNALPROCESS_H
#define INSIGHT_EXTERNALPROCESS_H


#include "base/boost_include.h"
#include "boost/process.hpp"
#include "base/cppextensions.h"



namespace insight {


class ExternalProcess
{
protected:
    std::unique_ptr<boost::process::child> process_;

public:
    ExternalProcess();

    ExternalProcess(std::unique_ptr<boost::process::child> process);

    template<class ...Args>
    ExternalProcess(Args&&... addArgs)
        : process_(
              std::make_unique<boost::process::child>(
                  std::forward<Args>(addArgs)...) )
    {}

    ~ExternalProcess();

    void wait();
    bool isRunning() const;

    const boost::process::child& process() const;
};


class Job;

typedef std::shared_ptr<Job> JobPtr;

class Job : public ExternalProcess
{
private:
  void read_start_out();
  void read_start_err();

public:
  boost::asio::io_service ios;
  boost::process::opstream in;
  boost::process::async_pipe out, err;
  boost::asio::streambuf buf_out, buf_err;

  std::function<void(const std::string& line)> processStdOut_, processStdErr_;

  Job();

  void runAndTransferOutput
  (
      std::vector<std::string>* stdoutbuffer = nullptr,
      std::vector<std::string>* stderrbuffer = nullptr
  );

  void ios_run_with_interruption(
      std::function<void(const std::string& line)> processStdOut,
      std::function<void(const std::string& line)> processStdErr );


  /**
   * @brief forkExternalProcess
   * @param job
   * @param child
   * the child needs to be created with the job's redirections !!
   */
  static void forkExternalProcess
  (
      JobPtr job,
      std::unique_ptr<boost::process::child> child
  );

  static JobPtr forkExternalProcess
  (
      const std::string& cmd,
      std::vector<std::string> argv = std::vector<std::string>()
  );

};




} // namespace insight

#endif // INSIGHT_EXTERNALPROCESS_H
