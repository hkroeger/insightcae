#ifndef ANALYZECLIENT_H
#define ANALYZECLIENT_H

#ifndef Q_MOC_RUN
#undef True
#undef False
#undef emit
#include "Wt/Json/Object.h"
#include "Wt/Http/Client.h"
#include "Wt/WIOService.h"
#endif

#include <string>
#include <system_error>
#include <future>

#include "base/parameterset.h"
#include "base/resultset.h"
#include "base/progressdisplayer.h"
#include "boost/variant.hpp"


namespace insight
{




class AnalyzeClient
{
public:
  typedef enum {
    None,
    SimpleRequest,
    QueryStatus,
    QueryResults,
    QueryExepath
  }
  CurrentRequestType;

  // success flag
  typedef std::function<void(bool)> ReportSuccessCallback;

  // success flag, progress state, results availability flag
  typedef std::function<void(bool, ProgressStatePtr, bool)> QueryStatusCallback;

  // success flag, result data
  typedef std::function<void(bool, ResultSetPtr)> QueryResultsCallback;

  // success flag, path
  typedef std::function<void(bool, boost::filesystem::path)> QueryExepathCallback;


protected:
  std::string url_;

  Wt::WIOService ioService_;
  Wt::Http::Client httpClient_;

  mutable boost::mutex mx_;
  CurrentRequestType crq_;

  boost::variant<
    QueryExepathCallback,
    ReportSuccessCallback,
    QueryStatusCallback,
    QueryResultsCallback
  > currentCallback_;

  void controlRequest(const std::string& action, AnalyzeClient::ReportSuccessCallback onCompletion);

  void handleHttpResponse(boost::system::error_code err, const Wt::Http::Message& response);

public:
  AnalyzeClient(const std::string& url);
  ~AnalyzeClient();

  bool waitForContact(int maxAttempts=20);

  bool isBusy() const;
  void forgetRequest();


  void queryExepath( QueryExepathCallback onExepathAvailable );

  void launchAnalysis(
      const ParameterSet& input,
      const boost::filesystem::path& parent_path,
      const std::string& analysisName,
      ReportSuccessCallback onCompletion
      );

  void queryStatus( QueryStatusCallback onStatusAvailable );

  void kill( ReportSuccessCallback onCompletion );
  void exit( ReportSuccessCallback onCompletion );
  void wnow( ReportSuccessCallback onCompletion );
  void wnowandstop( ReportSuccessCallback onCompletion );

  void queryResults( QueryResultsCallback onResultsAvailable );

};




}

#endif // ANALYZECLIENT_H
