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

  mx_.lock();
  auto crq=crq_;

  std::cout<<"httpResponse err="<<err<<", crq="<<crq<<", body="<<response.body()<<std::endl;

  bool success = (!err && response.status() == 200);

  switch (crq)
  {

    case SimpleRequest: {     
        crq_=None;
        mx_.unlock();
        boost::get<ReportSuccessCallback>(currentCallback_)(success);
      }
      break;

    case QueryStatus: {

        const auto *ct = response.getHeader("Content-Type");
        if (!ct)
        {
          crq_=None;
          mx_.unlock();
          throw insight::Exception("No content type specified in response!");
        }

        if ( success && ( (*ct)=="application/json") )
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
              boost::get<QueryStatusCallback>(currentCallback_)(success, ps, false);
            }
            crq_=None;
            mx_.unlock();
          }

          if (resultsAvailable)
          {
            crq_=None;
            mx_.unlock();
            boost::get<QueryStatusCallback>(currentCallback_)(success, ProgressStatePtr(), true);
          }
        }
        else
        {
          crq_=None;
          mx_.unlock();
          boost::get<QueryStatusCallback>(currentCallback_)(false, ProgressStatePtr(), false);
        }        
      }
      break;

    case QueryResults: {
//      std::cout<<"call"<<std::endl;
        const auto *ct = response.getHeader("Content-Type");
        if (!ct)
          throw insight::Exception("No content type specified in response!");
        if (success && ( (*ct)=="application/xml"))
        {
          auto body = response.body();
          ResultSetPtr r(new ResultSet(body));
          crq_=None;
          mx_.unlock();
          boost::get<QueryResultsCallback>(currentCallback_)(success, r);
        }
        else
        {
          crq_=None;
          mx_.unlock();
          boost::get<QueryResultsCallback>(currentCallback_)(false, ResultSetPtr());
        }
      }
      break;

    case None: {
        mx_.unlock();
        insight::Warning("Internal error: got unexpected response.");
      }
      break;

  }
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
