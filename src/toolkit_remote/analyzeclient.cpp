#include "analyzeclient.h"

#include "Wt/Json/Object.h"
#include "Wt/Json/Array.h"
#include "Wt/Json/Parser.h"
#include "Wt/Json/Serializer.h"

#include <functional>


namespace insight
{




void AnalyzeClient::controlRequest(const std::string &action, AnalyzeClient::ReportSuccessCallback onCompletion)
{
  boost::lock_guard<boost::mutex> mxg(mx_);

  if (crq_!=None)
    throw insight::Exception("There is an unfinished request!");

  crq_=SimpleRequest;

  currentCallback_=onCompletion;

  Wt::Http::Message msg;
  msg.setHeader("Content-Type", "application/json");
  Wt::Json::Object payload;
  payload["action"]=Wt::WString(action);
  msg.addBodyText(Wt::Json::serialize(payload));

  if (!httpClient_.post(url_, msg))
    throw insight::Exception("Could not launch control message!");
}



void AnalyzeClient::handleHttpResponse(boost::system::error_code err, const Wt::Http::Message &response)
{

  boost::lock_guard<boost::mutex> lock(mx_);


  std::string body=response.body();
  std::cout<<"httpResponse err="<<err<<", crq="<<crq_
          <<", body="<<body.substr(0, std::min<size_t>(80,body.size()))
         <<std::endl;

  bool success = (!err && response.status() == 200);

  switch (crq_)
  {

    case SimpleRequest: {     
        crq_=None;
        boost::thread( boost::get<ReportSuccessCallback>(currentCallback_), success ).detach();
      }
      break;

    case QueryStatus: {

      if (success)
      {
        const auto *ct = response.getHeader("Content-Type");
        if (!ct)
        {
          throw insight::Exception("No content type specified in response!");
        }

        if (  (*ct)=="application/json" )
        {
          Wt::Json::Object payload;
          Wt::Json::parse(response.body(), payload);

          bool resultsAvailable = payload["resultsAvailable"].toBool();
          std::cout<<"resultsavail="<<resultsAvailable<<std::endl;

          Wt::Json::Array states = payload.get("states");
          //for (const Wt::Json::Object& s: states)
          if (states.size()>0)
          {
            for (Wt::Json::Array::const_iterator i=states.begin(); i!=states.end(); i++)
            {
              //std::cout<<"state"<<std::endl;
              Wt::Json::Object s=*i;

              ProgressVariableList pvl;
              if (!s.isNull("ProgressVariableList"))
              {
                Wt::Json::Object pvs = s.get("ProgressVariableList");
                for (const auto& pv: pvs)
                {
                  //std::cout<<pv.first<<"="<<double(pv.second.toNumber())<<std::endl;;
                  pvl[pv.first]=pv.second.toNumber();
                }
              }
              //std::cout<<double(s.get("time").toNumber())<<":\""<<std::string(s.get("logMessage").toString())<<"\""<<std::endl;
              ProgressStatePtr ps(new ProgressState(
                s.get("time").toNumber(),
                pvl,
                s.get("logMessage").toString()
                ));
              boost::thread( boost::get<QueryStatusCallback>(currentCallback_), success, ps, false).detach();
            }
          }
          else
          {
            boost::thread( boost::get<QueryStatusCallback>(currentCallback_), true, ProgressStatePtr(), false).detach();
          }

          if (resultsAvailable)
          {
            boost::thread( boost::get<QueryStatusCallback>(currentCallback_), success, ProgressStatePtr(), true).detach();
          }
        }
        else
        {
          boost::thread( boost::get<QueryStatusCallback>(currentCallback_), false, ProgressStatePtr(), false).detach();
        }
      }
      else
      {
        boost::thread( boost::get<QueryStatusCallback>(currentCallback_), false, ProgressStatePtr(), false).detach();
      }
    } break;

    case QueryResults: {
      if (success)
      {
//      std::cout<<"call"<<std::endl;
        const auto *ct = response.getHeader("Content-Type");
        if (!ct)
          throw insight::Exception("No content type specified in response!");
        if ( (*ct)=="application/xml")
        {
          auto body = response.body();
          ResultSetPtr r(new ResultSet(body));
          boost::thread( boost::get<QueryResultsCallback>(currentCallback_), success, r).detach();
        }
        else
        {
          boost::thread( boost::get<QueryResultsCallback>(currentCallback_), false, ResultSetPtr()).detach();
        }
      }
      else
      {
        boost::thread( boost::get<QueryResultsCallback>(currentCallback_), false, ResultSetPtr()).detach();
      }
    } break;

    case QueryExepath: {
      if (success) {
        const auto *ct = response.getHeader("Content-Type");
        if (!ct)
          throw insight::Exception("No content type specified in response!");
        if ( (*ct)=="text/plain" )
        {
          boost::thread( boost::get<QueryExepathCallback>(currentCallback_), success, response.body()).detach();
        }
        else
        {
          boost::thread( boost::get<QueryExepathCallback>(currentCallback_), false, "").detach();
        }
      }
      else
      {
        boost::thread( boost::get<QueryExepathCallback>(currentCallback_), false, "").detach();
      }
    } break;

    case None: {
        insight::Warning("Internal error: got unexpected response.");
      }
      break;

  }

  crq_=None;
}




AnalyzeClient::AnalyzeClient(
    const std::string &url
    )
  : url_(url),
    ioService_(),
    httpClient_(ioService_),
    crq_(None)
{
  httpClient_.setMaximumResponseSize(512*1024*1024);
  httpClient_.setTimeout(std::chrono::seconds{24*3600});

  httpClient_.done().connect
      (
        std::bind(&AnalyzeClient::handleHttpResponse, this, std::placeholders::_1, std::placeholders::_2)
        );

  ioService_.start();
}



AnalyzeClient::~AnalyzeClient()
{
  ioService_.stop();
}

bool AnalyzeClient::isBusy() const
{
  boost::lock_guard<boost::mutex> mxg(mx_);
  return crq_!=None;
}

void AnalyzeClient::forgetRequest()
{
  boost::lock_guard<boost::mutex> mxg(mx_);
  crq_=None;
}

void AnalyzeClient::queryExepath(AnalyzeClient::QueryExepathCallback onExepathAvailable)
{
  boost::lock_guard<boost::mutex> mxg(mx_);

  if (crq_!=None)
    throw insight::Exception("There is an unfinished request!");

  crq_=QueryExepath;
  currentCallback_=onExepathAvailable;
  if (!httpClient_.get(url_+"/exepath"))
    throw insight::Exception("Could not query execution path of remote analysis!");
}




void AnalyzeClient::launchAnalysis(
    const ParameterSet& input,
    const boost::filesystem::path& parent_path,
    const std::string& analysisName,
    ReportSuccessCallback onCompletion
    )
{
  boost::lock_guard<boost::mutex> mxg(mx_);

  if (crq_!=None)
    throw insight::Exception("There is an unfinished request!");

  Wt::Http::Message msg;
  msg.setHeader("Content-Type", "application/xml");
  std::ostringstream cs;
  input.saveToStream(cs, parent_path, analysisName);
  msg.addBodyText(cs.str());

  crq_=SimpleRequest;
  currentCallback_=onCompletion;
  if (!httpClient_.post(url_, msg))
    throw insight::Exception("Could not launch remote analysis!");
}




void AnalyzeClient::queryStatus(AnalyzeClient::QueryStatusCallback onStatusAvailable)
{
  boost::lock_guard<boost::mutex> mxg(mx_);

  if (crq_!=None)
    throw insight::Exception("There is an unfinished request!");

  crq_=QueryStatus;
  currentCallback_=onStatusAvailable;
  if (!httpClient_.get(url_+"/latest"))
    throw insight::Exception("Could not query status of remote analysis!");
}




void AnalyzeClient::kill(AnalyzeClient::ReportSuccessCallback onCompletion)
{
  controlRequest("kill", onCompletion);
}

void AnalyzeClient::exit(AnalyzeClient::ReportSuccessCallback onCompletion)
{
  controlRequest("exit", onCompletion);
}

void AnalyzeClient::wnow(AnalyzeClient::ReportSuccessCallback onCompletion)
{
  controlRequest("wnow", onCompletion);
}

void AnalyzeClient::wnowandstop(AnalyzeClient::ReportSuccessCallback onCompletion)
{
  controlRequest("wnowandstop", onCompletion);
}




void AnalyzeClient::queryResults(AnalyzeClient::QueryResultsCallback onResultsAvailable)
{
  boost::lock_guard<boost::mutex> mxg(mx_);

  if (crq_!=None)
    throw insight::Exception("There is an unfinished request!");

  crq_=QueryResults;
  currentCallback_=onResultsAvailable;
  if (!httpClient_.get(url_+"/results"))
    throw insight::Exception("Could not query results of remote analysis!");
}




}
