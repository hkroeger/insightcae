#include "analysisthread.h"

#include "base/progressdisplayer/prefixedprogressdisplayer.h"


namespace insight {




// ====================================================================================
// ======== AnaylsisThread

void AnalysisThread::launch(std::function<void(void)> action)
{
  thread_ = boost::thread(

        [this,action](WarningDispatcher* globalWarning)
        {
          try
          {
            WarningDispatcher::getCurrent().setSuperDispatcher(globalWarning);

            action();

          }
          catch (...)
          {
            auto e = std::current_exception();
            if (exceptionHandler_)
              exceptionHandler_(e);
            else
              exception_ = e;
          }
        },

        &WarningDispatcher::getCurrent()
  );
}

AnalysisThread::AnalysisThread(
    AnalysisPtr analysis,
    ProgressDisplayer *pd,
    std::function<void(void)> preAction,
    std::function<void(void)> postAction,
    std::function<void(std::exception_ptr)> exHdlr
)
  : exceptionHandler_(exHdlr)
{
  launch(
        [this,analysis,pd,preAction,postAction]()
        {
          preAction();
          results_ = (*analysis)( *pd );
          postAction();
        }
  );
}

AnalysisThread::AnalysisThread
(
    std::function<void(void)> action,
    std::function<void(std::exception_ptr)> exHdlr
)
  : exceptionHandler_(exHdlr)
{
  launch(
        [action]()
        {
          action();
        }
  );
}

void AnalysisThread::interrupt()
{
  thread_.interrupt();
}

ResultSetPtr AnalysisThread::join()
{
  thread_.join();
  if (exception_) std::rethrow_exception(exception_);
  return results_;
}











AnalysisWorkerThread::AnalysisWorkerThread ( SynchronisedAnalysisQueue* queue, ProgressDisplayer* displayer )
    :
      displayer_(displayer),
      queue_(queue),
      mainThreadWarningDispatcher_(&WarningDispatcher::getCurrent())
{}




void AnalysisWorkerThread::operator() ()
{
  WarningDispatcher::getCurrent().setSuperDispatcher(mainThreadWarningDispatcher_);

  try
  {
    while ( !queue_->isEmpty() )
    {
      AnalysisInstance ai = queue_->dequeue();

      // run analysis and transfer results into given ResultSet object
      PrefixedProgressDisplayer pd(displayer_, ai.name,
                                   PrefixedProgressDisplayer::Prefixed,
                                   PrefixedProgressDisplayer::ParallelPrefix);

      try
      {
        auto& analysis= *(ai.analysis);
        ai.results->transfer( *analysis(pd) ); // call operator() from analysis object
      }
      catch ( const std::exception& e )
      {
        ai.exception = std::current_exception();
        WarningDispatcher::getCurrent().issue(
              "An exception has occurred while processing the instance "+ai.name+" of the parameter study."
              "The analsis of this instance was not completed.\n"
              "Reason: "+e.what()
              );
      }

      // Make sure we can be interrupted at least between analyses
      boost::this_thread::interruption_point();
    }
  }
  catch ( const std::exception& e )
  {
    exception_=std::current_exception();
  }
}




void AnalysisWorkerThread::rethrowIfNeeded() const
{
  if (exception_) std::rethrow_exception(exception_);
}




} // namespace insight
