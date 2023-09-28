#ifndef INSIGHT_EXTERNALPROCESS_H
#define INSIGHT_EXTERNALPROCESS_H


#include "base/boost_include.h"
#include "boost/process.hpp"
#include "base/cppextensions.h"
#include "base/outputanalyzer.h"



namespace insight {



class Job;

typedef std::shared_ptr<Job> JobPtr;

class Job
{

protected:
    boost::asio::io_service ios_;
    boost::process::opstream in_;
    boost::process::async_pipe out_, err_;
    boost::asio::streambuf buf_out_, buf_err_;

    std::unique_ptr<boost::process::child> process_;

    std::function<void(const std::string& line)> processStdOut_, processStdErr_;

    void read_start_out();
    void read_start_err();

public:


  Job();

  Job(
      const std::string& cmd,
      std::vector<std::string> argv = std::vector<std::string>()
  );

  Job(
    const std::pair<boost::filesystem::path,std::vector<std::string> >& commandAndArgs
  );

  ~Job();


  void forkProcess(
      std::unique_ptr<boost::process::child> child
  );

  void wait();
  bool isRunning() const;
  void terminate();

  std::ostream& input();
  void closeInput();

  const boost::process::child& process() const;

  void runAndTransferOutput
  (
      std::vector<std::string>* stdoutbuffer = nullptr,
      std::vector<std::string>* stderrbuffer = nullptr,
      bool mirrorStdout = true,
      bool mirrorStderr = true
  );

  void ios_run_with_interruption(
      std::function<void(const std::string& line)> processStdOut,
      std::function<void(const std::string& line)> processStdErr );

  void ios_run_with_interruption(
          OutputAnalyzer* oa = nullptr
          );
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
