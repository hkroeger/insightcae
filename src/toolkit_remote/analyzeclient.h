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

#ifndef ANALYZECLIENT_H
#define ANALYZECLIENT_H

#ifndef Q_MOC_RUN
#undef True
#undef False
#undef emit
#include <stdexcept>
#include "Wt/Json/Object.h"
#include "Wt/Http/Client.h"
#include "Wt/WIOService.h"
#endif

#include <string>
#include <system_error>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>

#include "base/parameterset.h"
#include "base/resultset.h"
#include "base/progressdisplayer.h"
#include "boost/variant.hpp"
#include "boost/asio/deadline_timer.hpp"


namespace insight
{




class AnalyzeClient;




/**
 * @brief The AnalyzeClientAction class
 * one request to the analyze server.
 *
 * Each action has exactly one outcome: either its result callback
 * or its timeout callback is called (both on the IO service of the client),
 * unless the client is destroyed before.
 */
class AnalyzeClientAction
    : public std::enable_shared_from_this<AnalyzeClientAction>
{
public:

    // success flag
    struct ReportSuccessResult
    {
      bool success = false;
    };
    typedef std::function<void(ReportSuccessResult)> ReportSuccessCallback;
    typedef std::function<void()> SimpleCallBack;

protected:
    AnalyzeClient& cl_;
    SimpleCallBack timeoutCallback_;
    std::chrono::milliseconds timeout_ {0}; // zero: timeout of the client

    /**
     * @brief doStart
     * send the request
     * @return false, if the request could not be sent
     */
    virtual bool doStart() =0;

    /**
     * @brief evaluateResponse
     * evaluate the response and report the result through the result callback.
     * Called at most once and not after a timeout. Must not throw.
     * err is set, if the request failed (also for responses which could not be sent).
     */
    virtual void evaluateResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response ) =0;

    /**
     * @brief post
     * execute f on the IO service of the client, unless it is shutting down
     */
    void post(std::function<void()> f);

private:
    boost::asio::deadline_timer deadline_;
    std::atomic<bool> finished_;

    /**
     * @brief tryFinish
     * @return true for the first caller only
     */
    bool tryFinish();

public:
    /**
     * @brief AnalyzeClientAction
     * @param cl
     * @param onTimeout
     * Will be called, if there is no response within the timeout of the client.
     * The request is aborted in that case.
     */
    AnalyzeClientAction(AnalyzeClient& cl, SimpleCallBack onTimeout );
    virtual ~AnalyzeClientAction();

    /**
     * @brief setTimeout
     * use a shorter timeout for this action than the one of the client
     * (longer values have no effect). To be called before the action is launched.
     */
    void setTimeout(std::chrono::milliseconds timeout);

    /**
     * @brief start
     * arm the timeout and send the request
     * @return false, if the request could not be sent
     */
    bool start();

    /**
     * @brief handleHttpResponse
     * called by the client for the response to this action's request
     */
    void handleHttpResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response );

    /**
     * @brief cancel
     * finish without calling any callback
     */
    void cancel();

    inline bool isFinished() const { return finished_; };
};




class QueryStatusAction : public AnalyzeClientAction
{
public:

    // success flag, progress state, results availability flag
    struct Result : public ReportSuccessResult
    {
      bool resultsAreAvailable = false;
      bool errorOccurred = false;
      std::shared_ptr<insight::Exception> exception;
    };
    typedef std::function<void(Result)> Callback;

private:
    Callback callback_;

protected:
    bool doStart() override;
    void evaluateResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;

public:
    QueryStatusAction(
            AnalyzeClient& cl,
            Callback callback,
            SimpleCallBack onTimeout );
};




class ControlRequestAction : public AnalyzeClientAction
{
private:
    std::string action_;
    ReportSuccessCallback callback_;

protected:
    bool doStart() override;
    void evaluateResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;

public:
    ControlRequestAction(
            AnalyzeClient& cl,
            const std::string& action,
            ReportSuccessCallback callback,
            SimpleCallBack onTimeout );
};




class LaunchAnalysisAction : public AnalyzeClientAction
{
private:
    Wt::Http::Message msg_;
    ReportSuccessCallback callback_;

protected:
    bool doStart() override;
    void evaluateResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;

public:
    LaunchAnalysisAction(
            AnalyzeClient& cl,
            const AnalysisParameterSet& input,
            const boost::filesystem::path& parent_path,
            ReportSuccessCallback callback,
            SimpleCallBack onTimeout );
};




class QueryResultsAction : public AnalyzeClientAction
{
public:

    // success flag, result data
    struct Result : public ReportSuccessResult
    {
      std::unique_ptr<ResultSet> results;
    };
    typedef std::function<void(Result)> Callback;

private:
    Callback callback_;

protected:
    bool doStart() override;
    void evaluateResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;

public:
    QueryResultsAction(
            AnalyzeClient& cl,
            Callback callback,
            SimpleCallBack onTimeout );
};




class QueryExepathAction : public AnalyzeClientAction
{
public:

    // success flag, path
    struct Result : public ReportSuccessResult
    {
      boost::filesystem::path exePath;
    };
    typedef std::function<void(Result)> Callback;

private:
    Callback callback_;

protected:
    bool doStart() override;
    void evaluateResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;

public:
    QueryExepathAction(
            AnalyzeClient& cl,
            Callback callback,
            SimpleCallBack onTimeout );
};




/**
 * @brief The AnalyzeClient class
 * asynchronous client for the REST API of "analyze --server".
 *
 * Requests are executed one after another: a request issued while
 * another one is in progress is queued.
 * All callbacks are executed on the client's IO service threads.
 */
class AnalyzeClient
{
  friend class AnalyzeClientAction;

protected:
  std::string analysisName_;
  std::string url_;

  Wt::WIOService ioService_;
  std::unique_ptr<Wt::Http::Client> httpClient_;

  mutable std::mutex mx_;
  std::condition_variable requestDone_;
  std::shared_ptr<AnalyzeClientAction> currentAction_;
  std::deque<std::shared_ptr<AnalyzeClientAction> > pendingActions_;
  bool requestInFlight_ = false;
  std::atomic<bool> shuttingDown_;
  bool shutDown_ = false;

  insight::ProgressDisplayer* progressDisplayer_;

  std::chrono::milliseconds timeout_ = std::chrono::minutes(15);

  void controlRequest( const std::string& action,
                       AnalyzeClientAction::ReportSuccessCallback onCompletion,
                       AnalyzeClientAction::SimpleCallBack onTimeout,
                       std::chrono::milliseconds timeout = std::chrono::milliseconds::zero() );

  void launchAction( std::shared_ptr<AnalyzeClientAction> action );

  /**
   * @brief startNextAction
   * start the next queued action, if no request is in progress
   */
  void startNextAction();

  void onHttpDone(
          boost::system::error_code err,
          const Wt::Http::Message& response );

public:
  AnalyzeClient(
      const std::string analysisName,
      const std::string& url,
      insight::ProgressDisplayer* progressDisplayer_
      );
  ~AnalyzeClient();

  /**
   * @brief shutdown
   * abort pending requests, stop the IO service and wait for running callbacks.
   * No callbacks are executed afterwards. Called by the destructor;
   * can be called before, to stop the client while it is still accessible.
   */
  void shutdown();

  /**
   * @brief setTimeout
   * maximum time to wait for the response to a request
   * (after which the timeout callback of the request is called)
   */
  void setTimeout(std::chrono::milliseconds timeout);
  std::chrono::milliseconds timeout() const;

  bool isBusy() const;

  /**
   * @brief forgetRequest
   * drop the current and all queued requests without calling their callbacks
   */
  void forgetRequest();

  void queryExepath(
          QueryExepathAction::Callback onExepathAvailable,
          AnalyzeClientAction::SimpleCallBack onTimeout );

  void launchAnalysis(
      const AnalysisParameterSet& input,
      const boost::filesystem::path& parent_path,
      AnalyzeClientAction::ReportSuccessCallback onCompletion,
      AnalyzeClientAction::SimpleCallBack onTimeout
      );

  void queryStatus( QueryStatusAction::Callback onStatusAvailable,
                    AnalyzeClientAction::SimpleCallBack onTimeout );

  /**
   * @param timeout
   * shorter timeout than the one of the client (zero: client's timeout)
   */
  void kill( AnalyzeClientAction::ReportSuccessCallback onCompletion,
             AnalyzeClientAction::SimpleCallBack onTimeout,
             std::chrono::milliseconds timeout = std::chrono::milliseconds::zero() );

  /**
   * @param timeout
   * shorter timeout than the one of the client (zero: client's timeout)
   */
  void exit( AnalyzeClientAction::ReportSuccessCallback onCompletion,
             AnalyzeClientAction::SimpleCallBack onTimeout,
             std::chrono::milliseconds timeout = std::chrono::milliseconds::zero() );

  void wnow( AnalyzeClientAction::ReportSuccessCallback onCompletion,
             AnalyzeClientAction::SimpleCallBack onTimeout );

  void wnowandstop( AnalyzeClientAction::ReportSuccessCallback onCompletion,
                    AnalyzeClientAction::SimpleCallBack onTimeout );

  void queryResults( QueryResultsAction::Callback onResultsAvailable,
                     AnalyzeClientAction::SimpleCallBack onTimeout );

  Wt::WIOService& ioService() { return ioService_; }
  Wt::Http::Client& httpClient() { return *httpClient_; }
  std::string analysisName() const { return analysisName_; }
  std::string url() const { return url_; }
  insight::ProgressDisplayer* progressDisplayer() const { return progressDisplayer_; }
  };




}

#endif // ANALYZECLIENT_H
