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
  insight::Analysis& analysis_;

  boost::mutex mx_;
  std::deque<insight::ProgressState> recordedStates_;

  insight::ResultSetPtr results_;

  boost::condition_variable rdcv_;

  double nextStateInfo(Wt::Json::Object& ro);

public:
  AnalyzeRESTServer(
      int argc, char *argv[],
      insight::Analysis& analysis
      );

  void setSolverThread(boost::thread* at);
  void setResults(insight::ResultSetPtr results);

  void update( const insight::ProgressState& pi ) override;

  bool hasResultsDelivered() const;
  void waitForResultDelivery();

protected:

    void handleRequest(
        const Wt::Http::Request &request,
        Wt::Http::Response &response
        ) override;
};


#endif // RESTAPI_H
