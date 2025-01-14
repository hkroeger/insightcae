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
{

  boost::thread thread_;

  mutable boost::mutex dataAccess_;
  boost::filesystem::path executionPath_;
  std::atomic<const Analysis*> analysis_;

protected:
  /**
   * @brief results_
   * stores the result set
   */
  ResultSetPtr results_;

  /**
   * @brief exception
   * stores the exception, if any occurred
   */
  std::exception_ptr exception_;

  std::function<void(std::exception_ptr)> exceptionHandler_;

  void launch(std::function<void(void)> action);

public:
  typedef
        std::tuple<const ParameterSet*, boost::filesystem::path>
        ParameterSetAndExePath;
  typedef
      boost::variant<
        ParameterSetAndExePath,
        insight::supplementedInputDataBasePtr>
      ParameterInput;

  AnalysisThread(
      const std::string& analysisName,
      const ParameterInput& input,
      ProgressDisplayer *pd
#ifndef SWIG
      ,
      std::function<void(void)> preAction = []()->void {},
      std::function<void(void)> postAction = []()->void {},
      std::function<void(std::exception_ptr)> exHdlr = std::function<void(std::exception_ptr)>()
#endif
  );

#ifndef SWIG
  AnalysisThread(
      std::function<void(void)> action,
      std::function<void(std::exception_ptr)> exHdlr = std::function<void(std::exception_ptr)>()
  );
#endif

  void interrupt();

  /**
   * @brief join
   * join thread and rethrow any exception, if there was no handler set
   * @return
   */
  ResultSetPtr join();

  template <class Rep, class Period>
  bool try_join_for(const boost::chrono::duration<Rep, Period>& rel_time)
  {
    return thread_.try_join_for(rel_time);
  }

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
