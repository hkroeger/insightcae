#include "analysisthread.h"

#include "base/analysis.h"
#include "base/progressdisplayer/prefixedprogressdisplayer.h"
#include <memory>
#include <mutex>

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
    const std::string& analysisName,
    const ParameterInput& input,
    ProgressDisplayer *pd,
    std::function<void(void)> preAction,
    std::function<void(void)> postAction,
    std::function<void(std::exception_ptr)> exHdlr
)
  : exceptionHandler_(exHdlr),
    analysis_(nullptr)
{
  launch(
        [this,analysisName,input,pd,preAction,postAction]()
        {
          preAction();

          std::shared_ptr<supplementedInputDataBase> sid;

          if (auto *pap=boost::get<ParameterSetAndExePath>(&input))
          {
              {
                boost::mutex::scoped_lock lck(dataAccess_);
                executionPath_ = std::get<1>(*pap);
              }
              sid =
                insight::Analysis::supplementedInputDatas()(
                    analysisName, ParameterSetInput(*std::get<0>(*pap)), executionPath_, *pd );
          }
          else if (auto *pap =
                     boost::get<insight::supplementedInputDataBasePtr>(
                         &input ) )
          {
              {
                  boost::mutex::scoped_lock lck(dataAccess_);
                  executionPath_ = (*pap)->executionPath();
              }
              sid = *pap;
          }

          auto analysis = insight::Analysis::analyses()(
              analysisName, sid);

          analysis_ = analysis.get();

          results_ = (*analysis)(*pd);

          postAction();
        }
  );
}

AnalysisThread::AnalysisThread
(
    std::function<void(void)> action,
    std::function<void(std::exception_ptr)> exHdlr
)
  : exceptionHandler_(exHdlr),
    analysis_(nullptr)
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

const boost::filesystem::path &AnalysisThread::executionPath() const
{
    boost::mutex::scoped_lock lck(dataAccess_);
    return executionPath_;
}

const Analysis *AnalysisThread::analysis() const
{
    return analysis_;
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
