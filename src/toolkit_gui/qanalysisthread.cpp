#include "qanalysisthread.h"

#include <QDebug>

namespace insight
{


void QAnalysisThread::launch(
    const std::string& analysisName,
    const ParameterInput& input,
    ProgressDisplayer *pd,
    std::function<void(void)> preAction,
    std::function<void(void)> postAction,
    std::function<void(std::exception_ptr)> exHdlr
    )
{
    AnalysisThread::launch
    (

        analysisName, input, pd,

        preAction,

        [this,postAction]()
        {
            postAction();
            Q_EMIT finished();
        },

        [this,exHdlr](std::exception_ptr e)  // exception handler
        {
            try
            {
                if (e) std::rethrow_exception(e);
            }
            catch (...)
            {
                Q_EMIT failed(e);
                exHdlr(e);
            }
        },

        [this]() // interrupt handler
        {
            Q_EMIT cancelled();
        }

    );
}


void QAnalysisThread::launch(
    std::function<void(void)> action,
    std::function<void(std::exception_ptr)> exHdlr
    )
{
    AnalysisThread::launch
        (
            [this,action]() {
                action();
                Q_EMIT finished();
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
        );
}

}
