#ifndef RESTAPI_H
#define RESTAPI_H

#include "boost/thread.hpp"
#include "base/analysis.h"
#include "base/progressdisplayer/textprogressdisplayer.h"

#include <Wt/WServer.h>
#include <Wt/WResource.h>

#include "Wt/Json/Object.h"



class AnalyzeRESTServer
: public Wt::WServer,
  public Wt::WResource,
  public insight::TextProgressDisplayer
{
  boost::thread* analysisThread_;
  insight::Analysis* analysis_;

  boost::mutex mx_;
  std::deque<insight::ProgressState> recordedStates_;

  insight::ResultSetPtr results_;
  std::string* inputFileContents_;

  boost::condition_variable wait_cv_;

  double nextStateInfo(Wt::Json::Object& ro);

public:
  AnalyzeRESTServer(
      const std::string& srvname,
      const std::string& listenAddr, int port
      );

  void setAnalysis(insight::Analysis* a);
  void setSolverThread(boost::thread* at);
  void setResults(insight::ResultSetPtr results);

  void update( const insight::ProgressState& pi ) override;

  bool hasResultsDelivered() const;
  void waitForResultDelivery();

  bool hasInputFileReceived() const;
  void waitForInputFile(std::string& inputFileContents_);

protected:

    void handleRequest(
        const Wt::Http::Request &request,
        Wt::Http::Response &response
        ) override;
};


#endif // RESTAPI_H
