#include "restapi.h"

#include "Wt/WServer.h"
#include "Wt/Http/Request.h"
#include "Wt/Http/Response.h"
#include "Wt/Json/Object.h"
#include "Wt/Json/Array.h"
#include "Wt/Json/Parser.h"
#include "Wt/Json/Serializer.h"

#include "boost/algorithm/string.hpp"

using namespace std;
using namespace Wt;


double AnalyzeRESTServer::nextStateInfo(
    Json::Object &s
    )
{
  auto& pi = recordedStates_.front();
  double t = pi.first; //copy (non-reference) for proper use as return value
  s["time"]=t;
  Wt::Json::Object pvl;
  for (const auto& cpv: pi.second)
  {
    pvl[cpv.first]=cpv.second;
  }
  s["ProgressVariableList"]=pvl;
  s["logMessage"]=Wt::WString(pi.logMessage_);

  recordedStates_.pop_front();
  return t;
}








AnalyzeRESTServer::AnalyzeRESTServer(
    int argc, char *argv[],
    insight::Analysis& a
    )
  : Wt::WServer(argv[0]),
    analysisThread_(nullptr),
    analysis_(a)
{

#warning needs parameter
  const char *cargv[]={argv[0], "--docroot", ".", "--http-listen", "localhost:8090"};
  setServerConfiguration(5, const_cast<char**>(cargv) );

  addResource(this, std::string());
}

void AnalyzeRESTServer::setSolverThread(boost::thread *at)
{
  analysisThread_=at;
}

void AnalyzeRESTServer::setResults(insight::ResultSetPtr results)
{
  results_=results;
}


void AnalyzeRESTServer::update(
    const insight::ProgressState& pi
    )
{
  TextProgressDisplayer::update(pi);
  mx_.lock();
  recordedStates_.push_back(pi);
  mx_.unlock();
}

bool AnalyzeRESTServer::hasResultsDelivered() const
{
  return !results_;
}

void AnalyzeRESTServer::waitForResultDelivery()
{
  boost::mutex::scoped_lock lock(mx_);
  while (!hasResultsDelivered())
    rdcv_.wait(lock);
}

void AnalyzeRESTServer::handleRequest(const Http::Request &request, Http::Response &response)
{

  response.addHeader("Server", "InsightCAE analyze");

  // extract payload with parameters
  Wt::Json::Object payload;
  std::string raw_payload(
        std::istreambuf_iterator<char>(request.in()),
        std::istreambuf_iterator<char>()
        );
  Wt::Json::parse(raw_payload, payload);



  if (request.method()=="GET")
  {
    std::cerr<<"status or results request"<<std::endl;


    auto whichState = payload.get("whichState");
    enum StateSelection { Next, All, Latest, Results } stateSelection = Next;
    if (!whichState.isNull())
    {
      std::string which = whichState.toString();
      boost::algorithm::to_lower(which);
      if (which=="next")
      {
        stateSelection = Next;
      }
      else if (which=="all")
      {
        stateSelection = All;
      }
      else if (which=="latest")
      {
        stateSelection = Latest;
      }
      else if (which=="results")
      {
        stateSelection = Results;
      }
    }

    if (stateSelection==Results)
    {
      if (results_)
      {
        response.setStatus(200);
        response.setMimeType("application/xml");
        results_->saveToStream( response.out() );

        {
          boost::mutex::scoped_lock lock(mx_);
          results_.reset();
          rdcv_.notify_one();
        }

        return;
      }
    }
    else
    {
      Wt::Json::Object res, state;
      Wt::Json::Array states;

      if (recordedStates_.size()>0)
      {
        if (stateSelection==Next)
        {
          nextStateInfo(state);
          states.push_back(state);
        }
        else if (stateSelection==All)
        {
          while (recordedStates_.size()>0)
          {
            nextStateInfo(state);
            states.push_back(state);
          }
        }
        else if (stateSelection==Latest)
        {
          // discard everything up to latest
          while (recordedStates_.size()>1)
          {
            recordedStates_.pop_front();
          }
          if (recordedStates_.size()>0)
          {
            nextStateInfo(state);
            states.push_back(state);
          }
        }
      }

      res["states"] = states;
      res["resultsAvailable"] = results_ ? true : false;

      response.setStatus(200);
      response.setMimeType("application/json");
      response.out()<<Wt::Json::serialize(res);

      return;
    }
  }
  else if (request.method()=="POST")
  {
    std::cerr<<"control request"<<std::endl;

    auto action_data = payload.get("action");
    if (!action_data.isNull())
    {
      std::string action = action_data.toString();
      boost::algorithm::to_lower(action);
      if (action=="kill")
      {
        if (analysisThread_)
        {
          analysisThread_->interrupt();
          response.setStatus(200);
          response.setMimeType("text/plain");
          response.out()<<"OK\n";
          return;
        }
      }
      else if (action=="wnow")
      {
        response.setStatus(200);
        response.setMimeType("text/plain");
        response.out()<<"OK\n";
        return;
      }
      else if (action=="wnowandstop")
      {
        response.setStatus(200);
        response.setMimeType("text/plain");
        response.out()<<"OK\n";
        return;
      }
    }

  }

  response.setStatus(400);
  response.setMimeType("text/plain");
  response.out()<<"Malformed request\n";
}
