#include "qanalysisthread.h"

#include <QDebug>

namespace insight
{


QAnalysisThread::QAnalysisThread(
    AnalysisPtr analysis,
    ProgressDisplayer* pd,
    std::function<void(void)> preAction,
    std::function<void(void)> postAction,
    std::function<void(std::exception_ptr)> exHdlr
    )
  : AnalysisThread
    (

      analysis, pd,

      preAction,

      [this,postAction]() {
        postAction();
        Q_EMIT finished(results_);
      },

      [this,exHdlr](std::exception_ptr e) {
        try
        {
          if (e) std::rethrow_exception(e);
        }
        catch (boost::thread_interrupted i)
        {
          Q_EMIT cancelled();
        }
        catch (...)
        {
          Q_EMIT failed(e);
          exHdlr(e);
        }
      }

    )
{}


QAnalysisThread::QAnalysisThread(
    std::function<void(void)> action,
    std::function<void(std::exception_ptr)> exHdlr
    )
  : AnalysisThread
    (
      [this,action]() {
        action();
        Q_EMIT finished(results_);
      },

      [this,exHdlr](std::exception_ptr e) {
        try
        {
          if (e) std::rethrow_exception(e);
        }
        catch (boost::thread_interrupted i)
        {
          Q_EMIT cancelled();
        }
        catch (...)
        {
          Q_EMIT failed(e);
          exHdlr(e);
        }
      }
    )
{}

}
