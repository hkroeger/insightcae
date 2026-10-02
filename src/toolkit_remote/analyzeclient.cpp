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
#include "base/exception.h"
#include "base/hierarchicalelement.h"
#include "base/warningdispatcher.h"

#include <functional>

using namespace std;

namespace insight
{




namespace
{

boost::system::error_code requestNotSentError()
{
    return boost::system::errc::make_error_code(boost::system::errc::io_error);
}

/**
 * @return content type of the response, empty if missing
 */
std::string contentType(const Wt::Http::Message& response)
{
    if (const auto *ct = response.getHeader("Content-Type"))
        return *ct;
    return std::string();
}

bool isSuccess(boost::system::error_code err, const Wt::Http::Message& response)
{
    return !err && response.status() == 200;
}

void logResponse(boost::system::error_code err, const Wt::Http::Message& response)
{
    bool success = isSuccess(err, response);

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

}




AnalyzeClientAction::AnalyzeClientAction(
        AnalyzeClient& cl,
        SimpleCallBack onTimeout )
    : cl_(cl),
      timeoutCallback_(onTimeout),
      deadline_(cl_.ioService()),
      finished_(false)
{}


AnalyzeClientAction::~AnalyzeClientAction()
{}


void AnalyzeClientAction::setTimeout(std::chrono::milliseconds timeout)
{
    timeout_ = timeout;
}


bool AnalyzeClientAction::tryFinish()
{
    bool expected=false;
    if (finished_.compare_exchange_strong(expected, true))
    {
        boost::system::error_code ec;
        deadline_.cancel(ec);
        return true;
    }
    return false;
}


void AnalyzeClientAction::post(std::function<void()> f)
{
    if (!cl_.shuttingDown_)
        cl_.ioService().post(f);
}


bool AnalyzeClientAction::start()
{
    auto self = shared_from_this();

    auto timeout = cl_.timeout();
    if (timeout_.count()>0 && timeout_<timeout)
        timeout = timeout_;
    deadline_.expires_from_now(
                boost::posix_time::milliseconds( timeout.count() ) );
    deadline_.async_wait(
                [self,this](const boost::system::error_code& ec)
                {
                    if (ec==boost::asio::error::operation_aborted)
                        return; // cancelled: finished in time

                    if (tryFinish())
                    {
                        dbg()<<"request timed out"<<std::endl;
                        {
                            // the client continues, once the aborted request is done
                            std::lock_guard<std::mutex> l(cl_.mx_);
                            if (!cl_.shuttingDown_)
                                cl_.httpClient_->abort();
                        }
                        if (timeoutCallback_)
                            post(timeoutCallback_);
                    }
                });

    bool ok=false;
    try
    {
        ok = doStart();
    }
    catch (const std::exception& e)
    {
        dbg()<<"could not send request: "<<e.what()<<std::endl;
        ok = false;
    }
    return ok;
}


void AnalyzeClientAction::handleHttpResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    if (!tryFinish())
        return; // timed out or cancelled before

    logResponse(err, response);

    try
    {
        evaluateResponse(err, response);
    }
    catch (const std::exception& e)
    {
        // must not happen, evaluateResponse should not throw
        insight::Warning("unexpected error while evaluating the response of the analysis server: %s", e.what());
    }
}


void AnalyzeClientAction::cancel()
{
    tryFinish();
}





QueryStatusAction::QueryStatusAction(
        AnalyzeClient& cl,
        QueryStatusAction::Callback queryStatusCallback,
        AnalyzeClientAction::SimpleCallBack onTimeout )
    : AnalyzeClientAction(cl, onTimeout),
      callback_(queryStatusCallback)
{}


bool QueryStatusAction::doStart()
{
    insight::CurrentExceptionContext ex("sending query status request");
    return cl_.httpClient().get(cl_.url()+"/all");
}


void QueryStatusAction::evaluateResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response )
{
    Result qsr;
    qsr.success = isSuccess(err, response);

    if (qsr.success)
    {
      try
      {
        auto ct = contentType(response);
        if (ct!="application/json")
        {
          throw insight::Exception(
                "unexpected content type of status response: \"%s\"", ct.c_str() );
        }

        Wt::Json::Object payload;
        Wt::Json::parse(response.body(), payload);

        qsr.resultsAreAvailable = payload["resultsAvailable"].toBool();

        if (cl_.progressDisplayer())
        {

          Wt::Json::Array states = payload.get("states");
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

          Wt::Json::Array progressStates = payload.get("progressStates");
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

          Wt::Json::Array logLines = payload.get("logLines");
          for (Wt::Json::Array::const_iterator i=logLines.begin(); i!=logLines.end(); i++)
          {
            cl_.progressDisplayer()->logMessage(
                    i->toString()
                  );
          }

        }

        if (payload["errorOccurred"].toBool())
        {
          qsr.errorOccurred=true;
          // the message must not be interpreted as format string
          std::string msg = payload.get("errorMessage").toString();
          qsr.exception=std::make_shared<insight::Exception>("%s", msg.c_str());
          std::string trace = payload.get("errorStackTrace").toString();
          qsr.exception->description()->strace_ = trace;
        }
      }
      catch (const std::exception& e)
      {
        dbg()<<"invalid status response: "<<e.what()<<std::endl;
        qsr = Result();
        qsr.success=false;
      }
    }

    auto cb=callback_;
    post([cb,qsr]() { cb(qsr); });
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


bool ControlRequestAction::doStart()
{
    Wt::Http::Message msg;
    msg.setHeader("Content-Type", "application/json");
    Wt::Json::Object payload;
    payload["action"]=Wt::WString(action_);
    msg.addBodyText(Wt::Json::serialize(payload));

    return cl_.httpClient().post(cl_.url(), msg);
}


void ControlRequestAction::evaluateResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    ReportSuccessResult rsr;
    rsr.success = isSuccess(err, response);

    auto cb=callback_;
    post([cb,rsr]() { cb(rsr); });
}





LaunchAnalysisAction::LaunchAnalysisAction(
        AnalyzeClient& cl,
        const AnalysisParameterSet& input,
        const boost::filesystem::path& parent_path,
        ReportSuccessCallback callback,
        AnalyzeClientAction::SimpleCallBack onTimeout )
    : AnalyzeClientAction(cl, onTimeout),
      callback_(callback)
{
    CurrentExceptionContext ex("composing parameter set message to server");
    msg_.setHeader("Content-Type", "application/xml");
    std::ostringstream cs;
    input.saveToStream(cs, hierarchicalData::Element::OutputProperties());
    msg_.addBodyText(cs.str());
}


bool LaunchAnalysisAction::doStart()
{
    insight::CurrentExceptionContext ex("sending launch analysis request");
    return cl_.httpClient().post(cl_.url(), msg_);
}


void LaunchAnalysisAction::evaluateResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    ReportSuccessResult rsr;
    rsr.success = isSuccess(err, response);

    auto cb=callback_;
    post([cb,rsr]() { cb(rsr); });
}





QueryResultsAction::QueryResultsAction(
        AnalyzeClient& cl,
        Callback callback,
        AnalyzeClientAction::SimpleCallBack onTimeout )
    : AnalyzeClientAction(cl, onTimeout),
      callback_(callback)
{}


bool QueryResultsAction::doStart()
{
    return cl_.httpClient().get(cl_.url()+"/results");
}


void QueryResultsAction::evaluateResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    auto qrr = std::make_shared<Result>();
    qrr->success = isSuccess(err, response);

    if (qrr->success)
    {
      try
      {
        auto ct = contentType(response);
        if (ct!="application/xml")
        {
          throw insight::Exception(
                "unexpected content type of results response: \"%s\"", ct.c_str() );
        }
        qrr->results = ResultSet::createFromString( response.body() );
      }
      catch (const std::exception& e)
      {
        dbg()<<"invalid results response: "<<e.what()<<std::endl;
        qrr->success=false;
        qrr->results.reset();
      }
    }

    auto cb=callback_;
    post([cb,qrr]() { cb(std::move(*qrr)); });
}





QueryExepathAction::QueryExepathAction(
        AnalyzeClient& cl,
        Callback callback,
        AnalyzeClientAction::SimpleCallBack onTimeout )
    : AnalyzeClientAction(cl, onTimeout),
      callback_(callback)
{}


bool QueryExepathAction::doStart()
{
    return cl_.httpClient().get(cl_.url()+"/exepath");
}


void QueryExepathAction::evaluateResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response )
{
    Result qer;
    qer.success = isSuccess(err, response);

    if (qer.success)
    {
      if ( contentType(response)=="text/plain" )
      {
        qer.exePath=response.body();
      }
      else
      {
        dbg()<<"unexpected content type of exepath response"<<std::endl;
        qer.success=false;
      }
    }

    auto cb=callback_;
    post([cb,qer]() { cb(qer); });
}





void AnalyzeClient::controlRequest(
        const std::string &action,
        AnalyzeClientAction::ReportSuccessCallback onCompletion,
        AnalyzeClientAction::SimpleCallBack onTimeout,
        std::chrono::milliseconds timeout )
{
  insight::CurrentExceptionContext ex("sending analyze client control request");

  auto a = std::make_shared<ControlRequestAction>(
                  *this, action,
                  onCompletion,
                  onTimeout );
  a->setTimeout(timeout);
  launchAction(a);
}



void AnalyzeClient::launchAction( std::shared_ptr<AnalyzeClientAction> action )
{
    insight::CurrentExceptionContext ex("launching analyze client action");
    {
        std::lock_guard<std::mutex> l(mx_);
        if (shuttingDown_) return;
        pendingActions_.push_back(action);
    }
    startNextAction();
}



void AnalyzeClient::startNextAction()
{
    while (true)
    {
        std::shared_ptr<AnalyzeClientAction> next;
        {
            std::lock_guard<std::mutex> l(mx_);
            if (shuttingDown_ || requestInFlight_ || pendingActions_.empty())
                return;
            if (currentAction_ && !currentAction_->isFinished())
                return;

            next = pendingActions_.front();
            pendingActions_.pop_front();
            currentAction_ = next;
            requestInFlight_ = true;
        }

        if (next->start())
            return; // response will arrive through onHttpDone

        // the request could not be sent: report failure and try the next one
        {
            std::lock_guard<std::mutex> l(mx_);
            requestInFlight_ = false;
        }
        next->handleHttpResponse(requestNotSentError(), Wt::Http::Message());
    }
}



void AnalyzeClient::onHttpDone(
        boost::system::error_code err,
        const Wt::Http::Message& response )
{
    std::shared_ptr<AnalyzeClientAction> action;
    {
        std::lock_guard<std::mutex> l(mx_);
        requestInFlight_ = false;
        action = currentAction_;
    }
    requestDone_.notify_all();

    if (action)
        action->handleHttpResponse(err, response);

    startNextAction();
}



AnalyzeClient::AnalyzeClient(
    const std::string analysisName,
    const std::string &url,
    insight::ProgressDisplayer* progressDisplayer
    )
  : analysisName_(analysisName),
    url_(url),
    ioService_(),
    httpClient_(std::make_unique<Wt::Http::Client>(ioService_)),
    shuttingDown_(false),
    progressDisplayer_(progressDisplayer)
{
  insight::CurrentExceptionContext ex("creating client for analysis execution at URL %s", url.c_str());

  httpClient_->setMaximumResponseSize(512*1024*1024);
  setTimeout(timeout_);

  httpClient_->done().connect(
        std::bind(&AnalyzeClient::onHttpDone, this,
                  std::placeholders::_1, std::placeholders::_2) );

  ioService_.start();
}




AnalyzeClient::~AnalyzeClient()
{
  shutdown();
}




void AnalyzeClient::shutdown()
{
  std::shared_ptr<AnalyzeClientAction> current;
  {
    std::lock_guard<std::mutex> l(mx_);
    if (shutDown_) return;
    shutDown_ = true;
    shuttingDown_ = true; // no more callbacks
    pendingActions_.clear();
    current = currentAction_;
  }
  if (current) current->cancel();

  // abort a pending request and wait until it is really finished:
  // the http client must not be destroyed, while its completion handler runs
  {
    std::unique_lock<std::mutex> l(mx_);
    if (requestInFlight_)
    {
      httpClient_->abort();
      requestDone_.wait_for(l, std::chrono::seconds(5), [this]{ return !requestInFlight_; });
    }
  }
  httpClient_.reset();

  ioService_.stop();
}




void AnalyzeClient::setTimeout(std::chrono::milliseconds timeout)
{
  timeout_ = timeout;
  // the own deadline of the actions shall expire first
  httpClient_->setTimeout( timeout + std::chrono::seconds(5) );
}




std::chrono::milliseconds AnalyzeClient::timeout() const
{
  return timeout_;
}




bool AnalyzeClient::isBusy() const
{
    std::lock_guard<std::mutex> l(mx_);
    return requestInFlight_
            || (currentAction_ && !currentAction_->isFinished())
            || !pendingActions_.empty();
}




void AnalyzeClient::forgetRequest()
{
    std::shared_ptr<AnalyzeClientAction> current;
    {
        std::lock_guard<std::mutex> l(mx_);
        pendingActions_.clear();
        current = currentAction_;
    }
    if (current) current->cancel();
}






void AnalyzeClient::queryExepath(
        QueryExepathAction::Callback onExepathAvailable,
        AnalyzeClientAction::SimpleCallBack onTimeout )
{
  launchAction( std::make_shared<QueryExepathAction>(
                      *this, onExepathAvailable, onTimeout ) );
}




void AnalyzeClient::launchAnalysis(
    const AnalysisParameterSet& input,
    const boost::filesystem::path& parent_path,
    AnalyzeClientAction::ReportSuccessCallback onCompletion,
    AnalyzeClientAction::SimpleCallBack onTimeout
    )
{
   launchAction( std::make_shared<LaunchAnalysisAction>(
                     *this,
                     input, parent_path,
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
        AnalyzeClientAction::SimpleCallBack onTimeout,
        std::chrono::milliseconds timeout )
{
  controlRequest("kill", onCompletion, onTimeout, timeout);
}




void AnalyzeClient::exit(
        AnalyzeClientAction::ReportSuccessCallback onCompletion,
        AnalyzeClientAction::SimpleCallBack onTimeout,
        std::chrono::milliseconds timeout )
{
  controlRequest("exit", onCompletion, onTimeout, timeout);
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
