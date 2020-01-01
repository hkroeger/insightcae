#include "analyzeclient.h"

#include "Wt/Json/Object.h"
#include "Wt/Json/Array.h"
#include "Wt/Json/Parser.h"
#include "Wt/Json/Serializer.h"

#include <functional>

namespace insight
{




void AnalyzeClient::handleHttpResponse(boost::system::error_code err, const Wt::Http::Message &response)
{

  bool success = (!err && response.status() == 200);

  switch (crq_)
  {

    case LaunchAnalysis:
    case Kill:
      boost::get<ReportSuccessCallback>(currentCallback_)(success);
      break;

    case QueryStatus:
      if (*response.getHeader("Content-Type")=="application/json")
      {
        Wt::Json::Object payload;
        Wt::Json::parse(response.body(), payload);
        bool resultsAvailable = payload["resultsAvailable"].toBool();
        Wt::Json::Array states = payload.get("states");
        for (const Wt::Json::Object& s: states)
        {
          ProgressVariableList pvl;
          Wt::Json::Object pvs = s.get("ProgressVariableList");
          for (const auto& pv: pvs)
          {
            pvl[pv.first]=pv.second.toNumber();
          }
          ProgressStatePtr ps(new ProgressState(
            s.get("time").toNumber(),
            pvl,
            s.get("logMessage").toString()
            ));
          boost::get<QueryStatusCallback>(currentCallback_)(success, ps, resultsAvailable);
        }
      }
      else
      {
        success=false;
        boost::get<QueryStatusCallback>(currentCallback_)(success, ProgressStatePtr(), false);
      }
      break;

    case QueryResults:
      if (*response.getHeader("Content-Type")=="application/xml")
      {
        ResultSetPtr r(new ResultSet(response.body()));
        boost::get<QueryResultsCallback>(currentCallback_)(success, r);
      }
      else
      {
        success=false;
        boost::get<QueryResultsCallback>(currentCallback_)(success, ResultSetPtr());
      }
      break;

    case None:
      insight::Warning("Internal error: got unexpected response.");
      break;

  }

  crq_=None;
}




AnalyzeClient::AnalyzeClient(
    const std::string &url
    )
  : url_(url),
    httpClient_(),
    crq_(None)
{
  httpClient_.done().connect
      (
        std::bind(&AnalyzeClient::handleHttpResponse, this, std::placeholders::_1, std::placeholders::_2)
        );
}




void AnalyzeClient::launchAnalysis(
    const ParameterSet& input,
    const boost::filesystem::path& parent_path,
    const std::string& analysisName,
    ReportSuccessCallback onCompletion
    )
{
  if (crq_!=None)
    throw insight::Exception("There is an unfinished request!");

  Wt::Http::Message msg;
  msg.setHeader("Content-Type", "application/xml");
  std::ostringstream cs;
  input.saveToStream(cs, parent_path, analysisName);
  msg.addBodyText(cs.str());

  crq_=LaunchAnalysis;
  currentCallback_=onCompletion;
  httpClient_.post(url_, msg);
}




void AnalyzeClient::queryStatus(AnalyzeClient::QueryStatusCallback onStatusAvailable)
{
  if (crq_!=None)
    throw insight::Exception("There is an unfinished request!");

  Wt::Http::Message msg;
  msg.setHeader("Content-Type", "application/json");
  std::ostringstream cs;

  Wt::Json::Object qp;
  qp["whichState"]="latest";
  msg.addBodyText( Wt::Json::serialize(qp) );

  crq_=QueryStatus;
  currentCallback_=onStatusAvailable;
  httpClient_.request(Wt::Http::Method::Get, url_, msg);
}




void AnalyzeClient::kill(AnalyzeClient::ReportSuccessCallback onCompletion)
{

}




void AnalyzeClient::queryResults(AnalyzeClient::QueryResultsCallback onResultsAvailable)
{

}




}
