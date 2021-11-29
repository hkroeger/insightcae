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
#include "Wt/Json/Object.h"
#include "Wt/Http/Client.h"
#include "Wt/WIOService.h"
#endif

#include <string>
#include <system_error>
#include <queue>
#include <condition_variable>

#include "base/parameterset.h"
#include "base/resultset.h"
#include "base/progressdisplayer.h"
#include "boost/variant.hpp"
#include "boost/asio/deadline_timer.hpp"


namespace insight
{




class AnalyzeClient;




class AnalyzeClientAction
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
    boost::asio::deadline_timer deadline_;
    SimpleCallBack timeoutCallback_;

private:
    Wt::Signals::Impl::Connection connection_;
    bool isFinished_;

    void setFinished();

public:
    /**
     * @brief AnalyzeClientAction
     * @param cl
     * @param onTimeout
     * Will be called in timeout event. Socket will be closed.
     */
    AnalyzeClientAction(AnalyzeClient& cl, SimpleCallBack onTimeout );
    virtual ~AnalyzeClientAction();

    virtual void start();
    virtual void handleHttpResponse(
            boost::system::error_code err,
            const Wt::Http::Message& response );

    inline bool isFinished() const { return isFinished_; };
};




class QueryStatusAction : public AnalyzeClientAction
{
public:

    // success flag, progress state, results availability flag
    struct Result : public ReportSuccessResult
    {
      bool resultsAreAvailable;
      bool errorOccurred;
      std::shared_ptr<insight::Exception> exception;
    };
    typedef std::function<void(Result)> Callback;

private:
    Callback callback_;

public:
    QueryStatusAction(
            AnalyzeClient& cl,
            Callback callback,
            SimpleCallBack onTimeout );

    void start() override;
    void handleHttpResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;
};




class ControlRequestAction : public AnalyzeClientAction
{
private:
    const std::string& action_;
    ReportSuccessCallback callback_;

public:
    ControlRequestAction(
            AnalyzeClient& cl,
            const std::string& action,
            ReportSuccessCallback callback,
            SimpleCallBack onTimeout );

    void start() override;
    void handleHttpResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;
};




class LaunchAnalysisAction : public AnalyzeClientAction
{
private:
    Wt::Http::Message msg_;
    ReportSuccessCallback callback_;

public:
    LaunchAnalysisAction(
            AnalyzeClient& cl,
            const ParameterSet& input,
            const boost::filesystem::path& parent_path,
            const std::string& analysisName,
            ReportSuccessCallback callback,
            SimpleCallBack onTimeout );

    void start() override;
    void handleHttpResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;
};




class QueryResultsAction : public AnalyzeClientAction
{
public:

    // success flag, result data
    struct Result : public ReportSuccessResult
    {
      ResultSetPtr results;
    };
    typedef std::function<void(Result)> Callback;

private:
    Callback callback_;

public:
    QueryResultsAction(
            AnalyzeClient& cl,
            Callback callback,
            SimpleCallBack onTimeout );

    void start() override;
    void handleHttpResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;
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

public:
    QueryExepathAction(
            AnalyzeClient& cl,
            Callback callback,
            SimpleCallBack onTimeout );

    void start() override;
    void handleHttpResponse(
                boost::system::error_code err,
                const Wt::Http::Message& response ) override;
};





class AnalyzeClient
{

protected:
  std::string analysisName_;
  std::string url_;

  Wt::WIOService ioService_;
  Wt::Http::Client httpClient_;

  std::shared_ptr<AnalyzeClientAction> currentAction_;

  insight::ProgressDisplayer* progressDisplayer_;

  int timeout_ = 60*10;

  void controlRequest( const std::string& action,
                       AnalyzeClientAction::ReportSuccessCallback onCompletion,
                       AnalyzeClientAction::SimpleCallBack onTimeout );

  void launchAction( std::shared_ptr<AnalyzeClientAction> action );

public:
  AnalyzeClient(
      const std::string analysisName,
      const std::string& url,
      insight::ProgressDisplayer* progressDisplayer_
      );
  ~AnalyzeClient();


//  void waitForContact(AnalyzeClientAction::SimpleCallBack onContact,
//                      AnalyzeClientAction::SimpleCallBack onNoContact,
//                      int maxAttempts=20);

  bool isBusy() const;
  void forgetRequest();

  void queryExepath(
          QueryExepathAction::Callback onExepathAvailable,
          AnalyzeClientAction::SimpleCallBack onTimeout );

  void launchAnalysis(
      const ParameterSet& input,
      const boost::filesystem::path& parent_path,
      const std::string& analysisName,
      AnalyzeClientAction::ReportSuccessCallback onCompletion,
      AnalyzeClientAction::SimpleCallBack onTimeout
      );

  void queryStatus( QueryStatusAction::Callback onStatusAvailable,
                    AnalyzeClientAction::SimpleCallBack onTimeout );

  void kill( AnalyzeClientAction::ReportSuccessCallback onCompletion,
             AnalyzeClientAction::SimpleCallBack onTimeout );

  void exit( AnalyzeClientAction::ReportSuccessCallback onCompletion,
             AnalyzeClientAction::SimpleCallBack onTimeout );

  void wnow( AnalyzeClientAction::ReportSuccessCallback onCompletion,
             AnalyzeClientAction::SimpleCallBack onTimeout );

  void wnowandstop( AnalyzeClientAction::ReportSuccessCallback onCompletion,
                    AnalyzeClientAction::SimpleCallBack onTimeout );

  void queryResults( QueryResultsAction::Callback onResultsAvailable,
                     AnalyzeClientAction::SimpleCallBack onTimeout );

  Wt::WIOService& ioService() { return ioService_; }
  Wt::Http::Client& httpClient() { return httpClient_; }
  std::string analysisName() const { return analysisName_; }
  std::string url() const { return url_; }
  insight::ProgressDisplayer* progressDisplayer() const { return progressDisplayer_; }
  };




}

#endif // ANALYZECLIENT_H
