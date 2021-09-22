#include "restapi.h"

#include "Wt/WServer.h"
#include "Wt/Http/Request.h"
#include "Wt/Http/Response.h"
#include "Wt/Json/Object.h"
#include "Wt/Json/Array.h"
#include "Wt/Json/Parser.h"
#include "Wt/Json/Serializer.h"

#include "boost/algorithm/string.hpp"

#include <csignal>

using namespace std;
using namespace Wt;




void globalSignalHandler(int sig);

class SignalChecker;

SignalChecker* currentSignalHandler = nullptr;

class SignalChecker
{
#ifndef WIN32
  boost::condition_variable& cv_;
  boost::mutex& mx_;
  int& signal_;

  __sighandler_t
    old_SIGHUP_,
    old_SIGINT_,
    old_SIGPIPE_,
    old_SIGTERM_;
#endif

public:

  void signalHandler(int signal)
  {
#ifndef WIN32
    boost::mutex::scoped_lock lock(mx_);
    signal_=signal;
    cout<<"Got signal "<<signal<<endl;
    cv_.notify_all();
#endif
  }

  SignalChecker(boost::condition_variable& cv, boost::mutex& mx, int& signal)
#ifndef WIN32
    : cv_(cv),
      mx_(mx),
      signal_(signal)
#endif
  {
#ifndef WIN32
    if (currentSignalHandler)
      throw std::logic_error("Internal error: There is already another signal checker active!");

    currentSignalHandler=this;
    old_SIGHUP_ = std::signal(SIGHUP, globalSignalHandler);
    old_SIGINT_ = std::signal(SIGINT, globalSignalHandler);
    old_SIGPIPE_ = std::signal(SIGPIPE, globalSignalHandler);
    old_SIGTERM_ = std::signal(SIGTERM, globalSignalHandler);
#endif
  }

  ~SignalChecker()
  {
#ifndef WIN32
    // restore
    std::signal(SIGHUP, old_SIGHUP_);
    std::signal(SIGINT, old_SIGINT_);
    std::signal(SIGPIPE, old_SIGPIPE_);
    std::signal(SIGTERM, old_SIGTERM_);
#endif
  }
};



void globalSignalHandler(int sig)
{
  currentSignalHandler->signalHandler(sig);
}



std::pair<double,Json::Object> AnalyzeRESTServer::nextStateInfo()
{
  Json::Object s;
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

  return std::make_pair(t, s);
}


Json::Object AnalyzeRESTServer::nextProgressInfo()
{
  Json::Object s;

  auto& pi = recordedProgressStates_.front();

  s["path"]=Wt::WString(pi.path);
  if (auto *dbl=boost::get<double>(&pi.value))
  {
    s["double"]=*dbl;
  }
  else if (auto *text=boost::get<std::string>(&pi.value))
  {
    s["text"]=Wt::WString(*text);
  }
  else
  {
    // blank
  }

  recordedProgressStates_.pop_front();

  return s;
}







AnalyzeRESTServer::AnalyzeRESTServer(
    const std::string& srvname,
    const std::string& listenAddr, int port
    )
  : Wt::WServer(srvname.c_str()),
    analysisThread_(nullptr),
    analysis_(nullptr)
{

  auto addr = boost::str(boost::format(listenAddr+":%d") % port);
  const char *cargv[]={
    srvname.c_str(),
    "--docroot", ".",
    "--http-listen", addr.c_str()
  };

  setServerConfiguration(5, const_cast<char**>(cargv) );

  addResource(this, std::string());
  addResource(this, "/next");
  addResource(this, "/all");
  addResource(this, "/latest");
  addResource(this, "/results");
  addResource(this, "/exepath");
}


void AnalyzeRESTServer::setAnalysis(insight::Analysis *a)
{
  analysis_=a;
}

void AnalyzeRESTServer::setSolverThread(insight::AnalysisThread *at)
{
  analysisThread_=at;
}

void AnalyzeRESTServer::setResults(insight::ResultSetPtr results)
{
  results_=results;
}

void AnalyzeRESTServer::setException(const insight::Exception &ex)
{
  exception_=std::make_shared<insight::Exception>(ex);
}




void AnalyzeRESTServer::setActionProgressValue(const string &path, double value)
{
  TextProgressDisplayer::setActionProgressValue(path, value);
  mx_.lock();
  recordedProgressStates_.push_back( ProgressState{ path, value } );
  mx_.unlock();
}




void AnalyzeRESTServer::setMessageText(const string &path, const string &message)
{
  TextProgressDisplayer::setMessageText(path, message);
  mx_.lock();
  recordedProgressStates_.push_back( ProgressState{ path, message } );
  mx_.unlock();
}




void AnalyzeRESTServer::finishActionProgress(const string &path)
{
  TextProgressDisplayer::finishActionProgress(path);
  mx_.lock();
  recordedProgressStates_.push_back( ProgressState{ path, boost::blank() } );
  mx_.unlock();
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




bool AnalyzeRESTServer::hasInputFileReceived() const
{
  return !inputFileContents_->empty();
}

bool AnalyzeRESTServer::waitForInputFile(std::string& inputFileContents)
{
  inputFileContents_ = &inputFileContents;

  int sig=0;
  SignalChecker sighld(wait_cv_, mx_, sig);

  boost::mutex::scoped_lock lock(mx_);
  while (!( hasInputFileReceived() || (sig!=0) ))
  {
    wait_cv_.wait(lock);
  }

  return sig==0;
}


bool AnalyzeRESTServer::hasResultsDelivered() const
{
  return !results_;
}

bool AnalyzeRESTServer::waitForResultDelivery()
{
  boost::mutex::scoped_lock lock(mx_);
  int sig=-1;
  SignalChecker sighld(wait_cv_, mx_, sig);
  while ( !hasResultsDelivered() && (sig==0) )
  {
    wait_cv_.wait(lock);
  }
  return sig==0;
}

void AnalyzeRESTServer::handleRequest(const Http::Request &request, Http::Response &response)
{

  response.addHeader("Server", "InsightCAE analyze");

  // extract payload with parameters
  Wt::Json::Object payload;
  if (request.contentType()=="application/json")
  {
    std::string raw_payload(
          std::istreambuf_iterator<char>(request.in()),
          std::istreambuf_iterator<char>()
          );
    Wt::Json::parse(raw_payload, payload);
  }



  if (request.method()=="GET")
  {
    std::cerr<<"status or results request"<<std::endl;


    //auto whichState = payload.get("whichState");
    std::string which = request.path();
    cout<<"which="<<which<<endl;
    enum StateSelection { Next, All, Latest, Results, ExePath } stateSelection = Next;
//    if (!whichState.isNull())
    {
//      std::string which = whichState.toString();
      boost::algorithm::to_lower(which);
      if (which=="/next")
      {
        stateSelection = Next;
      }
      else if (which=="/all")
      {
        stateSelection = All;
      }
      else if (which=="/latest")
      {
        stateSelection = Latest;
      }
      else if (which=="/results")
      {
        stateSelection = Results;
      }
      else if (which=="/exepath")
      {
        stateSelection = ExePath;
      }
    }

    if (stateSelection==Results)
    {
      if (results_)
      {
        response.setStatus(200);
        response.setMimeType("application/xml");
        results_->saveToStream( response.out() );

        return;
      }
    }
    else if (stateSelection==ExePath)
    {
      response.setStatus(200);
      response.setMimeType("text/plain");
      response.out() << analysis_->executionPath();

      return;
    }
    else
    {
      Wt::Json::Array states, progressStates;

      if (recordedStates_.size()>0)
      {
        if (stateSelection==Next)
        {
          states.push_back( nextStateInfo().second );
        }
        else if (stateSelection==All)
        {
          while (recordedStates_.size()>0)
          {
            states.push_back( nextStateInfo().second );
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
            states.push_back( nextStateInfo().second );
          }
        }
      }

      while (recordedProgressStates_.size()>0)
      {
        progressStates.push_back( nextProgressInfo() );
      }

      Wt::Json::Object res;
      res["states"] = states;
      res["progressStates"] = progressStates;
      res["inputFileReceived"] = hasInputFileReceived();
      res["resultsAvailable"] = results_ ? true : false;
      res["errorOccurred"] = exception_ ? true : false;
      res["errorMessage"] = exception_ ? exception_->what() : "";
      res["errorStackTrace"] = exception_ ? exception_->strace().c_str() : "";

      response.setStatus(200);
      response.setMimeType("application/json");
      response.out()<<Wt::Json::serialize(res);

      return;
    }
  }
  else if (request.method()=="POST")
  {
    std::cerr<<"control request"<<std::endl;

    if (request.contentType()=="application/xml")
    {
      // input file
      insight::readStreamIntoString(request.in(), *inputFileContents_);


      response.setStatus(200);
      response.setMimeType("text/plain");
      response.out()<<"OK\n";

      {
        boost::mutex::scoped_lock lock(mx_);
        results_.reset();
        wait_cv_.notify_all();
      }

      return;
    }
    else
    {
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
        else if (action=="exit")
        {
          if (analysisThread_)
          {
            analysisThread_->interrupt();
          }
          scheduleStop();

          response.setStatus(200);
          response.setMimeType("text/plain");
          response.out()<<"OK\n";
          return;
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

  }

  response.setStatus(400);
  response.setMimeType("text/plain");
  response.out()<<"Malformed request\n";
}
