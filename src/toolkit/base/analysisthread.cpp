#include "analysisthread.h"

#include "base/analysis.h"
#include "base/progressdisplayer/prefixedprogressdisplayer.h"
#include <exception>
#include <memory>
#include <mutex>




namespace insight {




// ====================================================================================
// ======== AnalysisThread


AnalysisThread::AnalysisThread(
    const std::string& analysisName,
    const ParameterInput& input,
    ProgressDisplayer *pd,
    std::function<void(void)> preAction,
    std::function<void(void)> postAction,
    ExceptionHandler exHdlr,
    InterruptHandler intHdlr
)
  : analysis_(nullptr),
    Thread(
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
                  auto psp=std::get<0>(*pap);
                  sid =
                      insight::Analysis::supplementedInputDatas()(
                          analysisName,
                          ParameterSetInput(*psp),
                          executionPath_,
                          *pd->forkNewAction(99, "Processing input data") );
              }
              else if (auto *sibp =
                       boost::get<insight::supplementedInputDataBasePtr>(
                           &input ) )
              {
                  {
                      boost::mutex::scoped_lock lck(dataAccess_);
                      executionPath_ = (*sibp)->executionPath();
                  }
                  sid = *sibp;
              }

              auto analysis = insight::Analysis::analyses()(
                  analysisName, sid);

              analysis_ = analysis.get();

              this->ResultSetPtr::operator=( (*analysis)(*pd) );

              postAction();
          },
          exHdlr, intHdlr)
{}




AnalysisThread::AnalysisThread
(
    std::function<void(void)> action,
    ExceptionHandler exHdlr,
    InterruptHandler intHdlr
)
  : analysis_(nullptr),
    Thread( action, exHdlr, intHdlr )
{}




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

          std::cout<<"name "<<ai.name<<std::endl;
        auto result = analysis(pd);

        ai.results=std::move(result);
      }
      catch ( std::exception& e )
      {
          std::cout<<"name2 "<<ai.name<<std::endl;
        // ai.exception = std::current_exception();
        WarningDispatcher::getCurrent().issue(
              "An exception has occurred while processing the instance "+ai.name+" of the parameter study."
              "The analsis of this instance was not completed.\n"
              "Reason: "+e.what()
              );
        ai.exception = std::make_exception_ptr(e);
      }

      queue_->storeProcessed(std::move(ai));

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
