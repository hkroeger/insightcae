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

#ifndef INSIGHT_ANALYSISTHREAD_H
#define INSIGHT_ANALYSISTHREAD_H




#include "base/analysis.h"
#include "base/insightthread.h"
#include "base/supplementedinputdata.h"
#include "boost/filesystem/path.hpp"




namespace insight {


/**
 * @brief The AnalysisThread class
 * This class wraps the execution of an analysis in a separate thread.
 * It is made sure, the warnings and progress info is properly propagated
 * to the main thread.
 */
class AnalysisThread
    : public std::unique_ptr<insight::ResultSet>,
      public insight::Thread
{

  mutable boost::mutex dataAccess_;
  boost::filesystem::path executionPath_;
  std::atomic<const Analysis*> analysis_;

public:
  typedef
        std::tuple<const ParameterSet*, boost::filesystem::path>
        ParameterSetAndExePath;
  typedef
      boost::variant<
        ParameterSetAndExePath,
        insight::supplementedInputDataBasePtr>
      ParameterInput;

  typedef std::function<void(std::exception_ptr)> ExceptionHandler;
  typedef std::function<void(void)> InterruptHandler;

  AnalysisThread();

  void launch(
      const std::string& analysisName,
      ParameterInput input,
      ProgressDisplayer *pd
#ifndef SWIG
      ,
      std::function<void(void)> preAction = []()->void {},
      std::function<void(void)> postAction = []()->void {},
      ExceptionHandler exHdlr = ExceptionHandler(),
      InterruptHandler intHdlr = InterruptHandler()
#endif
      );

#ifndef SWIG
  void launch(
      std::function<void(void)> action,
      ExceptionHandler exHdlr = ExceptionHandler(),
      InterruptHandler intHdlr = InterruptHandler()
      );
#endif

  const boost::filesystem::path& executionPath() const;
  const Analysis* analysis() const;
};




/**
 * @brief The AnalysisWorkerThread class
 * Objects of this class work together with the SynchronizedAnalysisQueue.
 * The latter holds a pool of Analyses to process.
 * For each processor, an AnalysisWorkerThread object is created.
 * It grabs an Analysis form the queue, processes it and grabs the next until none is left.
 */
class AnalysisWorkerThread
    : boost::noncopyable
{
protected:
    ProgressDisplayer* displayer_;
    SynchronisedAnalysisQueue* queue_;

    /**
     * @brief exception
     * stores the exception, if any occurred
     */
    std::exception_ptr exception_;

    WarningDispatcher* mainThreadWarningDispatcher_;

public:
    /**
     * @brief AnalysisWorkerThread
     * @param queue
     * @param displayer
     * Constructs the worker.
     * This is expected to be executed in the main thread.
     */
    AnalysisWorkerThread ( SynchronisedAnalysisQueue* queue, ProgressDisplayer* displayer=nullptr );

    /**
     * @brief operator ()
     * Executes the job.
     * This function runs in a separate thread.
     */
    void operator() ();

    void rethrowIfNeeded() const;
};




} // namespace insight

#endif // INSIGHT_ANALYSISTHREAD_H
