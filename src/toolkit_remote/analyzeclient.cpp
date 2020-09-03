/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "analyzeclient.h"

#include "Wt/Json/Object.h"
#include "Wt/Json/Array.h"
#include "Wt/Json/Parser.h"
#include "Wt/Json/Serializer.h"

#include <functional>

using namespace std;

namespace insight
{




//void TaskQueue::post(TaskQueue::Job job)
//{
//  std::unique_lock<std::mutex> lck(mx_);

//  jobQueue_.push(job);

//  lck.unlock();
//  cv_.notify_all();

//}


//void TaskQueue::dispatchJobs()
//{
//  std::unique_lock<std::mutex> lck(mx_);

//  while (true)
//  {
//    //Wait until we have jobs
//    cv_.wait(lck, [this] {
//        return (jobQueue_.size());
//    });

//    while (jobQueue_.size()>0)
//    {
//      auto op = std::move(jobQueue_.front());
//      jobQueue_.pop();

//      lck.unlock();
//      op();
//      lck.lock();

//      boost::this_thread::interruption_point();
//    }
//  }
//}





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

  try
  {
    bool success = (!err && response.status() == 200);

    std::string body=response.body();
    CurrentRequestType crq=crq_;
    crq_=None;

    std::cout<<"httpResponse err="<<err<<", crq="<<crq_
             <<", status="<<response.status()
             <<", success="<<success
             <<", body="<<body.substr(0, std::min<size_t>(80,body.size()))
             <<std::endl;

    switch (crq)
    {

      case SimpleRequest: {
//          tq_.post(
//                [=]()
//                {
                  boost::get<ReportSuccessCallback>(currentCallback_)(success);
//                }
//          );
        }
        break;

      case QueryStatus: {

//        ProgressStatePtrList pss;
        bool resultsAvailable = false;

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

            resultsAvailable = payload["resultsAvailable"].toBool();

            if (progressDisplayer_)
            {
              Wt::Json::Array states = payload.get("states");
              if (states.size()>0)
              {
                for (Wt::Json::Array::const_iterator i=states.begin(); i!=states.end(); i++)
                {
                  Wt::Json::Object s=*i;

                  ProgressVariableList pvl;
                  if (!s.isNull("ProgressVariableList"))
                  {
                    Wt::Json::Object pvs = s.get("ProgressVariableList");
                    for (const auto& pv: pvs)
                    {
                      pvl[pv.first]=pv.second.toNumber();
                    }
                  }

                  progressDisplayer_->update(
                        ProgressState(
                          s.get("time").toNumber(),
                          pvl,
                          s.get("logMessage").toString()
                          )
                        );
                }
              }

              Wt::Json::Array progressStates = payload.get("progressStates");
              if (progressStates.size()>0)
              {
                for (auto i=progressStates.begin();
                     i!=progressStates.end(); i++)
                {
                  Wt::Json::Object s=*i;
                  auto path = s.get("path").toString();
                  if (!s.isNull("double"))
                  {
                    double value = s.get("double").toNumber();
                    progressDisplayer_->setActionProgressValue(path, value);
                  }
                  else if (!s.isNull("text"))
                  {
                    string text = s.get("text").toString();
                    progressDisplayer_->setMessageText(path, text);
                  }
                  else
                  {
                    progressDisplayer_->finishActionProgress(path);
                  }
                }
              }
            }
          }
          else
          {
            success=false;
          }
        }

//        tq_.post( [=]()
//        {
          boost::get<QueryStatusCallback>(currentCallback_)(success, resultsAvailable);
//        });

      } break;

      case QueryResults: {
        ResultSetPtr r;

        if (success)
        {
          const auto *ct = response.getHeader("Content-Type");

          if (!ct)
            throw insight::Exception("No content type specified in response!");

          if ( (*ct)=="application/xml")
          {
            auto body = response.body();
            r.reset(new ResultSet(body));
          }
        }

//        tq_.post( [=]()
//        {
          boost::get<QueryResultsCallback>(currentCallback_)(success, r);
//        });

      } break;

      case QueryExepath: {
        std::string exepath;

        if (success)
        {

          const auto *ct = response.getHeader("Content-Type");

          if (!ct)
            throw insight::Exception("No content type specified in response!");

          if ( (*ct)=="text/plain" )
          {
            exepath=response.body();
          }
          else
          {
            success=false;
          }
        }

//        tq_.post( [=]()
//        {
          boost::get<QueryExepathCallback>(currentCallback_)(success, exepath);
//        });

      } break;

      case None: {
          insight::Warning("Internal error: got unexpected response.");
        }
        break;

    }
  }
  catch (const std::exception&)
  {
    auto ex = std::current_exception();
    if (exHdlr_)
      exHdlr_(ex);
    else
      std::rethrow_exception(ex);
  }
}




AnalyzeClient::AnalyzeClient(
    const std::string &url,
    insight::ProgressDisplayer* progressDisplayer,
    ExceptionHandler exceptionHandler
    )
  : url_(url),
    ioService_(),
    httpClient_(ioService_),
    crq_(None),
    progressDisplayer_(progressDisplayer),
    exHdlr_(exceptionHandler)
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
  return ( crq_!=None );
}




void AnalyzeClient::forgetRequest()
{
  boost::lock_guard<boost::mutex> mxg(mx_);
  crq_=None;
}




bool AnalyzeClient::waitForContact(int maxAttempts)
{
  // wait for server to come up and respond
  std::mutex m;
  std::condition_variable cv;
  bool contacted=false;
  int attempts=0;

  do
  {
    attempts++;

    queryStatus(
          [&](bool success, bool)
          {
            std::unique_lock<std::mutex> lck(m);
            if (success)
            {
              contacted=true;
            }
            cv.notify_all();
          }
    );

    {
     std::unique_lock<std::mutex> lk(m);
     cv.wait_for(lk, std::chrono::seconds(10) );
    }

    if (!contacted)
    {
      forgetRequest();
      boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));
    }
  }
  while( !(contacted || (attempts > maxAttempts)) );

  return contacted;
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
