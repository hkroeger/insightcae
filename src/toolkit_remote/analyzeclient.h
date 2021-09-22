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


namespace insight
{




///**
// * @brief The TaskQueue class
// * takes jobs, executes them one by one in the thread, from which dispatchJobs() was called
// */
//class TaskQueue
//{
//public:
//  typedef std::function<void(void)> Job;

//protected:
//  std::mutex mx_;
//  std::condition_variable cv_;
//  std::queue<Job> jobQueue_;


//public:
//  void post(Job job);
//  void dispatchJobs();
//};




class AnalyzeClient
{
public:
  typedef enum {
    None,
    SimpleRequest,
    QueryStatus,
    QueryResults,
    QueryExepath
  }
  CurrentRequestType;

  // success flag
  struct ReportSuccessResult
  {
    bool success = false;
  };
  typedef std::function<void(ReportSuccessResult)> ReportSuccessCallback;

  // success flag, progress state, results availability flag
  struct QueryStatusResult : public ReportSuccessResult
  {
    bool resultsAreAvailable;
    bool errorOccurred;
    std::shared_ptr<insight::Exception> exception;
  };
  typedef std::function<void(QueryStatusResult)> QueryStatusCallback;

  // success flag, result data
  struct QueryResultsResult : public ReportSuccessResult
  {
    ResultSetPtr results;
  };
  typedef std::function<void(QueryResultsResult)> QueryResultsCallback;

  // success flag, path
  struct QueryExepathResult : public ReportSuccessResult
  {
    boost::filesystem::path exePath;
  };
  typedef std::function<void(QueryExepathResult)> QueryExepathCallback;

  typedef std::function<void(std::exception_ptr)> ExceptionHandler;

protected:
  std::string analysisName_;
  std::string url_;

  Wt::WIOService ioService_;
  Wt::Http::Client httpClient_;

  mutable boost::mutex mx_;
  std::atomic<CurrentRequestType> crq_;

  boost::variant<
    QueryExepathCallback,
    ReportSuccessCallback,
    QueryStatusCallback,
    QueryResultsCallback
  > currentCallback_;

//  TaskQueue tq_;
  ExceptionHandler exHdlr_;

  insight::ProgressDisplayer* progressDisplayer_;

  int timeout_ = 60*10;

  void controlRequest(const std::string& action, AnalyzeClient::ReportSuccessCallback onCompletion);

  void handleHttpResponse(boost::system::error_code err, const Wt::Http::Message& response);

  template <class ResultType>
  ResultType syncRun( std::function< void(std::function<void(ResultType)>) > function, bool throwOnNoSuccess = true)
  {
    std::mutex m;
    std::condition_variable cv;
    bool received=false;
    ResultType result;

    function(
          [&](ResultType r)
          {
            std::unique_lock<std::mutex> lck(m);
            received=true;
            result=r;
            cv.notify_all();
          }
    );

    {
     std::unique_lock<std::mutex> lk(m);
     cv.wait_for(lk, std::chrono::seconds(timeout_) );
    }

    if (!received)
      throw insight::Exception( boost::str(boost::format(
                  "no response from the remote server was received within %d seconds!"
            ) % timeout_ ) );

    if (!result.success && throwOnNoSuccess)
      throw insight::Exception("the query returned no success");

    return result;
  }


public:
  AnalyzeClient(
      const std::string analysisName,
      const std::string& url,
      insight::ProgressDisplayer* progressDisplayer_
#ifndef SWIG
      ,
      ExceptionHandler exceptionHandler = [](std::exception_ptr)->void {}
#endif
      );
  ~AnalyzeClient();

  bool waitForContact(int maxAttempts=20);

  bool isBusy() const;
  void forgetRequest();

  void queryExepath( QueryExepathCallback onExepathAvailable );

  void launchAnalysis(
      const ParameterSet& input,
      const boost::filesystem::path& parent_path,
      const std::string& analysisName,
      ReportSuccessCallback onCompletion
      );
  ReportSuccessResult launchAnalysisSync(
      const ParameterSet& input,
      const boost::filesystem::path& parent_path,
      const std::string& analysisName,
      bool throwOnNoSuccess = true
      );

  void queryStatus( QueryStatusCallback onStatusAvailable ); // async
  QueryStatusResult queryStatusSync(bool throwOnNoSuccess = true); //sync

  void kill( ReportSuccessCallback onCompletion );
  ReportSuccessResult killSync(bool throwOnNoSuccess = true);

  void exit( ReportSuccessCallback onCompletion );
  ReportSuccessResult exitSync(bool throwOnNoSuccess = true);

  void wnow( ReportSuccessCallback onCompletion );
  ReportSuccessResult wnowSync(bool throwOnNoSuccess = true);

  void wnowandstop( ReportSuccessCallback onCompletion );
  ReportSuccessResult wnowandstopSync(bool throwOnNoSuccess = true);

  void queryResults( QueryResultsCallback onResultsAvailable );
  QueryResultsResult queryResultsSync(bool throwOnNoSuccess = true);

};




}

#endif // ANALYZECLIENT_H
