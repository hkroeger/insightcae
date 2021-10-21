#ifndef INSIGHT_ANALYSISTHREAD_H
#define INSIGHT_ANALYSISTHREAD_H




#include "base/analysis.h"




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
  AnalysisThread(
      AnalysisPtr analysis,
      ProgressDisplayer* pd
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
