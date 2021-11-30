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




void AnalyzeClientAction::setFinished()
{
    isFinished_=true;
    deadline_.cancel();
    connection_.disconnect();
}


AnalyzeClientAction::AnalyzeClientAction(
        AnalyzeClient& cl,
        SimpleCallBack onTimeout )
    : cl_(cl),
      isFinished_(false),
      deadline_(cl_.ioService()),
      timeoutCallback_(onTimeout)
{
    connection_ = cl_.httpClient().done().connect
      (
        std::bind(&AnalyzeClientAction::handleHttpResponse, this,
                  std::placeholders::_1, std::placeholders::_2)
        );
}


AnalyzeClientAction::~AnalyzeClientAction()
{}


void AnalyzeClientAction::start()
{
    deadline_.expires_from_now(
                boost::posix_time::milliseconds(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        cl_.httpClient().timeout() ).count() )
                 );
    deadline_.async_wait(
                [this](boost::system::error_code)
                {
                    timeoutCallback_();
                });
}


void AnalyzeClientAction::handleHttpResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    setFinished();

    bool success = (!err && response.status() == 200);

    dbg() <<   "httpResponse err="<<err
          << ", status="<<response.status()
          << ", success="<<(success?"true":"false")
          << std::endl;

    if (success)
    {
        std::string body=response.body();
        auto&ds=dbg();
        ds << "body=";
        if (body.size()<1024)
            ds<<body;
        else
        {
            ds<<body.substr(0, 512)<<"\n...\n"<<body.substr(body.size()-512, body.size());
        }
        ds<<std::endl;
    }

}





QueryStatusAction::QueryStatusAction(
        AnalyzeClient& cl,
        QueryStatusAction::Callback queryStatusCallback,
        AnalyzeClientAction::SimpleCallBack onTimeout )
    : AnalyzeClientAction(cl, onTimeout),
      callback_(queryStatusCallback)
{}


void QueryStatusAction::start()
{
    if (!cl_.httpClient().get(cl_.url()+"/all"))
        throw insight::Exception("Could not query status of remote analysis!");
}


void QueryStatusAction::handleHttpResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response )
{
    AnalyzeClientAction::handleHttpResponse(err, response);
    bool success = (!err && response.status() == 200);

    bool resultsAvailable = false;
    bool errorOccurred = false;
    std::shared_ptr<insight::Exception> exception_;

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

        if (cl_.progressDisplayer())
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

              cl_.progressDisplayer()->update(
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
                cl_.progressDisplayer()->setActionProgressValue(path, value);
              }
              else if (!s.isNull("text"))
              {
                string text = s.get("text").toString();
                cl_.progressDisplayer()->setMessageText(path, text);
              }
              else
              {
                cl_.progressDisplayer()->finishActionProgress(path);
              }
            }
          }

          Wt::Json::Array logLines = payload.get("logLines");
          if (logLines.size()>0)
          {
            for (Wt::Json::Array::const_iterator i=logLines.begin(); i!=logLines.end(); i++)
            {
              cl_.progressDisplayer()->logMessage(
                      i->toString()
                    );
            }
          }

        }

        if (payload["errorOccurred"].toBool())
        {
          errorOccurred=true;
          exception_=std::make_shared<insight::Exception>(
                std::string(payload.get("errorMessage").toString()),
                std::string(payload.get("errorStackTrace").toString())
                );
        }

      }
      else
      {
        success=false;
      }
    }

    Result qsr;
    qsr.success=success;
    qsr.resultsAreAvailable=resultsAvailable;
    qsr.errorOccurred=errorOccurred;
    qsr.exception=exception_;

    cl_.ioService().post( std::bind(callback_, qsr) );
}





ControlRequestAction::ControlRequestAction(
        AnalyzeClient& cl,
        const std::string& action,
        ReportSuccessCallback callback,
        AnalyzeClientAction::SimpleCallBack onTimeout )
    : AnalyzeClientAction(cl, onTimeout),
      action_(action),
      callback_(callback)
{}


void ControlRequestAction::start()
{
    Wt::Http::Message msg;
    msg.setHeader("Content-Type", "application/json");
    Wt::Json::Object payload;
    payload["action"]=Wt::WString(action_);
    msg.addBodyText(Wt::Json::serialize(payload));

    if (!cl_.httpClient().post(cl_.url(), msg))
      throw insight::Exception("Could not launch control message!");
}


void ControlRequestAction::handleHttpResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    AnalyzeClientAction::handleHttpResponse(err, response);

    ReportSuccessResult rsr;
    rsr.success = (!err && response.status() == 200);

    cl_.ioService().post( std::bind(callback_, rsr) );
}





LaunchAnalysisAction::LaunchAnalysisAction(
        AnalyzeClient& cl,
        const ParameterSet& input,
        const boost::filesystem::path& parent_path,
        const std::string& analysisName,
        ReportSuccessCallback callback,
        AnalyzeClientAction::SimpleCallBack onTimeout )
    : AnalyzeClientAction(cl, onTimeout),
      callback_(callback)
{
    CurrentExceptionContext ex("composing parameter set message to server");
    msg_.setHeader("Content-Type", "application/xml");
    std::ostringstream cs;
    input.saveToStream(cs, parent_path, analysisName);
    msg_.addBodyText(cs.str());
}


void LaunchAnalysisAction::start()
{
    if (!cl_.httpClient().post(cl_.url(), msg_))
      throw insight::Exception("Could not launch remote analysis!");
}


void LaunchAnalysisAction::handleHttpResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    AnalyzeClientAction::handleHttpResponse(err, response);

    ReportSuccessResult rsr;
    rsr.success = (!err && response.status() == 200);

    cl_.ioService().post( std::bind(callback_, rsr) );
}





QueryResultsAction::QueryResultsAction(
        AnalyzeClient& cl,
        Callback callback,
        AnalyzeClientAction::SimpleCallBack onTimeout )
    : AnalyzeClientAction(cl, onTimeout),
      callback_(callback)
{}


void QueryResultsAction::start()
{
    if (!cl_.httpClient().get(cl_.url()+"/results"))
      throw insight::Exception("Could not query results of remote analysis!");
}


void QueryResultsAction::handleHttpResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    AnalyzeClientAction::handleHttpResponse(err, response);
    bool success = (!err && response.status() == 200);

    ResultSetPtr r;

    if (success)
    {
      const auto *ct = response.getHeader("Content-Type");

      if (!ct)
        throw insight::Exception("No content type specified in response!");

      if ( (*ct)=="application/xml")
      {
        r = ResultSet::createFromString( response.body(), cl_.analysisName() );
      }
    }

    Result qrr;
    qrr.success=success;
    qrr.results=r;
    cl_.ioService().post( std::bind(callback_, qrr) );
}





QueryExepathAction::QueryExepathAction(
        AnalyzeClient& cl,
        Callback callback,
        AnalyzeClientAction::SimpleCallBack onTimeout )
    : AnalyzeClientAction(cl, onTimeout),
      callback_(callback)
{}


void QueryExepathAction::start()
{
    if (!cl_.httpClient().get(cl_.url()+"/exepath"))
      throw insight::Exception("Could not query execution path of remote analysis!");
}


void QueryExepathAction::handleHttpResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    AnalyzeClientAction::handleHttpResponse(err, response);
    bool success = (!err && response.status() == 200);

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

    Result qer;
    qer.success=success;
    qer.exePath=exepath;
    cl_.ioService().post( std::bind(callback_, qer) );
}





void AnalyzeClient::controlRequest(
        const std::string &action,
        AnalyzeClientAction::ReportSuccessCallback onCompletion,
        AnalyzeClientAction::SimpleCallBack onTimeout )
{
  launchAction( std::make_shared<ControlRequestAction>( *this, action, onCompletion, onTimeout ) );
}



void AnalyzeClient::launchAction( std::shared_ptr<AnalyzeClientAction> action )
{
    if ( isBusy() )
        throw insight::Exception( "internal error: there is already a transaction in progress" );

    currentAction_ = action;
    currentAction_->start();
}



AnalyzeClient::AnalyzeClient(
    const std::string analysisName,
    const std::string &url,
    insight::ProgressDisplayer* progressDisplayer
    )
  : analysisName_(analysisName),
    url_(url),
    ioService_(),
    httpClient_(ioService_),
    progressDisplayer_(progressDisplayer)
{
  httpClient_.setMaximumResponseSize(512*1024*1024);
  httpClient_.setTimeout(std::chrono::seconds{15*60});

  ioService_.start();
}




AnalyzeClient::~AnalyzeClient()
{
  ioService_.stop();
}




bool AnalyzeClient::isBusy() const
{
    if ( currentAction_ )
    {
        if (!currentAction_->isFinished())
        {
            return true;
        }
    }
    return false;
}




void AnalyzeClient::forgetRequest()
{
  currentAction_.reset();
}




//void AnalyzeClient::waitForContact(
//        AnalyzeClientAction::SimpleCallBack contactCallback,
//        AnalyzeClientAction::SimpleCallBack noContactCallback,
//        int maxAttempts )
//{

//    auto scheduleNextAttempt = [this,contactCallback,noContactCallback,maxAttempts]() {
//        // schedule next attempt in 10 secs, if some remain
//        if (maxAttempts>0)
//        {
//            ioService().schedule(
//                    std::chrono::seconds(2),
//                    std::bind( &AnalyzeClient::waitForContact, this,
//                               contactCallback, noContactCallback,
//                               maxAttempts-1 ) );
//        }
//        else
//        {
//            // cancel otherwise
//            ioService().post( noContactCallback );
//        }

//    };

//    queryStatus(
//                [this,contactCallback,scheduleNextAttempt](QueryStatusAction::Result r)
//                {
//                    if (r.success)
//                    {
//                        // execute callback on success
//                        ioService().post( contactCallback );
//                    }
//                    else
//                    {
//                        scheduleNextAttempt();
//                    }
//                },

//                scheduleNextAttempt
//    );
//}




void AnalyzeClient::queryExepath(
        QueryExepathAction::Callback onExepathAvailable,
        AnalyzeClientAction::SimpleCallBack onTimeout )
{
  launchAction( std::make_shared<QueryExepathAction>(
                      *this, onExepathAvailable, onTimeout ) );
}




void AnalyzeClient::launchAnalysis(
    const ParameterSet& input,
    const boost::filesystem::path& parent_path,
    const std::string& analysisName,
    AnalyzeClientAction::ReportSuccessCallback onCompletion,
    AnalyzeClientAction::SimpleCallBack onTimeout
    )
{
   launchAction( std::make_shared<LaunchAnalysisAction>(
                     *this,
                     input, parent_path, analysisName,
                     onCompletion, onTimeout ) );
}




void AnalyzeClient::queryStatus(
        QueryStatusAction::Callback onStatusAvailable,
        AnalyzeClientAction::SimpleCallBack onTimeout )
{
    launchAction( std::make_shared<QueryStatusAction>(
                      *this, onStatusAvailable, onTimeout ) );
}




void AnalyzeClient::kill(
        AnalyzeClientAction::ReportSuccessCallback onCompletion,
        AnalyzeClientAction::SimpleCallBack onTimeout)
{
  controlRequest("kill", onCompletion, onTimeout );
}




void AnalyzeClient::exit(
        AnalyzeClientAction::ReportSuccessCallback onCompletion,
        AnalyzeClientAction::SimpleCallBack onTimeout )
{
  controlRequest("exit", onCompletion, onTimeout);
}




void AnalyzeClient::wnow(
        AnalyzeClientAction::ReportSuccessCallback onCompletion,
        AnalyzeClientAction::SimpleCallBack onTimeout )
{
  controlRequest("wnow", onCompletion, onTimeout);
}


void AnalyzeClient::wnowandstop(
        AnalyzeClientAction::ReportSuccessCallback onCompletion,
        AnalyzeClientAction::SimpleCallBack onTimeout )
{
  controlRequest("wnowandstop", onCompletion, onTimeout);
}


void AnalyzeClient::queryResults(
        QueryResultsAction::Callback onResultsAvailable,
        AnalyzeClientAction::SimpleCallBack onTimeout )
{
  launchAction( std::make_shared<QueryResultsAction>(
                    *this, onResultsAvailable, onTimeout ) );

}



}
