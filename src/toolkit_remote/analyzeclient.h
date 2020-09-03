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
  typedef std::function<void(bool)> ReportSuccessCallback;

  // success flag, progress state, results availability flag
  typedef std::function<void(bool, bool)> QueryStatusCallback;

  // success flag, result data
  typedef std::function<void(bool, ResultSetPtr)> QueryResultsCallback;

  // success flag, path
  typedef std::function<void(bool, boost::filesystem::path)> QueryExepathCallback;

  typedef std::function<void(std::exception_ptr)> ExceptionHandler;

protected:
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

  void controlRequest(const std::string& action, AnalyzeClient::ReportSuccessCallback onCompletion);

  void handleHttpResponse(boost::system::error_code err, const Wt::Http::Message& response);

public:
  AnalyzeClient(
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

  void queryStatus( QueryStatusCallback onStatusAvailable );

  void kill( ReportSuccessCallback onCompletion );
  void exit( ReportSuccessCallback onCompletion );
  void wnow( ReportSuccessCallback onCompletion );
  void wnowandstop( ReportSuccessCallback onCompletion );

  void queryResults( QueryResultsCallback onResultsAvailable );

};




}

#endif // ANALYZECLIENT_H
