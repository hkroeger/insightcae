#ifndef INSIGHT_EXTERNALPROCESS_H
#define INSIGHT_EXTERNALPROCESS_H


#include <boost/filesystem.hpp>
#include "boost/process.hpp"
#include "base/cppextensions.h"
#include "base/outputanalyzer.h"



namespace insight {



class Job;

typedef std::shared_ptr<Job> JobPtr;

class Job
{

protected:
    boost::process::opstream in_;
    boost::process::ipstream out_, err_;

    std::unique_ptr<boost::process::child> process_;

    std::function<void(const std::string& line)> processStdOut_, processStdErr_;


public:
  Job();

  Job(
      const std::string& cmd,
      std::vector<std::string> argv = std::vector<std::string>(),
      bool searchCmdInPath = true
  );

  Job(
    const std::pair<boost::filesystem::path,std::vector<std::string> >& commandAndArgs,
    bool searchCmdInPath = true
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
