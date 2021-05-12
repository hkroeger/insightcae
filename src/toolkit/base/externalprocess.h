#ifndef INSIGHT_EXTERNALPROCESS_H
#define INSIGHT_EXTERNALPROCESS_H


#include "base/boost_include.h"
#include "boost/process.hpp"



namespace insight {




struct Job
{
private:
  void read_start_out();
  void read_start_err();

public:
  boost::asio::io_service ios;
  boost::process::opstream in;
  boost::process::async_pipe out, err;
  boost::asio::streambuf buf_out, buf_err;

  std::shared_ptr<boost::process::child> process;

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
};




typedef std::shared_ptr<Job> JobPtr;



JobPtr forkExternalProcess
(
    const std::string& cmd,
    std::vector<std::string> argv = std::vector<std::string>()
);


} // namespace insight

#endif // INSIGHT_EXTERNALPROCESS_H
