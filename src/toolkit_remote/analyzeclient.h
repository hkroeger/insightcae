#ifndef ANALYZECLIENT_H
#define ANALYZECLIENT_H

#include <string>
#include <system_error>

#include "base/parameterset.h"
#include "base/resultset.h"
#include "base/progressdisplayer.h"
#include "boost/variant.hpp"

#include "Wt/Json/Object.h"
#include "Wt/Http/Client.h"

namespace insight
{




class AnalyzeClient
{
public:
  typedef enum {
    None,
    LaunchAnalysis,
    QueryStatus,
    Kill,
    QueryResults
  }
  CurrentRequestType;

  // success flag
  typedef std::function<void(bool)> ReportSuccessCallback;

  // success flag, progress state, results availability flag
  typedef std::function<void(bool, ProgressStatePtr, bool)> QueryStatusCallback;

  // success flag, result data
  typedef std::function<void(bool, ResultSetPtr)> QueryResultsCallback;


protected:
  std::string url_;

  Wt::Http::Client httpClient_;

  CurrentRequestType crq_;

  boost::variant<
    ReportSuccessCallback,
    QueryStatusCallback,
    QueryResultsCallback
  > currentCallback_;

  void handleHttpResponse(boost::system::error_code err, const Wt::Http::Message& response);

public:
  AnalyzeClient(const std::string& url);

  void launchAnalysis(
      const ParameterSet& input,
      const boost::filesystem::path& parent_path,
      const std::string& analysisName,
      ReportSuccessCallback onCompletion
      );

  void queryStatus( QueryStatusCallback onStatusAvailable );

  void kill( ReportSuccessCallback onCompletion );

  void queryResults( QueryResultsCallback onResultsAvailable );

};




}

#endif // ANALYZECLIENT_H
