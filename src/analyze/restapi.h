#ifndef RESTAPI_H
#define RESTAPI_H

#include "boost/thread.hpp"
#include "base/analysis.h"
#include "base/analysisthread.h"
#include "base/progressdisplayer/textprogressdisplayer.h"

#include <Wt/WServer.h>
#include <Wt/WResource.h>

#include "Wt/Json/Object.h"


class AnalyzeRESTServer
: public Wt::WServer,
  public Wt::WResource,
  public insight::TextProgressDisplayer
{
  insight::AnalysisThread* analysisThread_;
  boost::filesystem::path inputFileParentPath_;

  boost::mutex mx_;
  std::deque<insight::ProgressState> recordedStates_;
  std::deque<std::string> logLines_;

  typedef
    boost::variant<
      boost::blank,
      double,
      std::string
    > ProgressStateValue;

  struct ProgressState
  {
    std::string path;
    ProgressStateValue value;
  };
  std::deque<ProgressState> recordedProgressStates_;

  std::shared_ptr<insight::Exception> exception_;
  insight::ResultSetPtr results_;
  // std::string* inputFileContents_;

  boost::condition_variable wait_cv_;

  std::pair<double,Wt::Json::Object> nextStateInfo();
  Wt::Json::Object nextProgressInfo();
  Wt::WString nextLogLine();

public:
  AnalyzeRESTServer(
      const std::string& srvname,
      const std::string& listenAddr, int port
      );


  void setAnalysis(const boost::filesystem::path& inputFileParentPath);
  void setSolverThread(insight::AnalysisThread* at);
  void setResults(insight::ResultSetPtr results);
  void setException(const insight::Exception& ex);

  void update( const insight::ProgressState& pi ) override;
  void logMessage(const std::string& line) override;
  void setActionProgressValue(const std::string &path, double value) override;
  void setMessageText(const std::string &path, const std::string& message) override;
  void finishActionProgress(const std::string &path) override;

  bool hasResultsDelivered() const;
  bool waitForResultDelivery();

  // bool hasInputFileReceived() const;
  // bool waitForInputFile(std::string& inputFileContents_);

protected:

    void handleRequest(
        const Wt::Http::Request &request,
        Wt::Http::Response &response
        ) override;
};


#endif // RESTAPI_H
