#include "iqiscadscriptjob.h"

#include "iqcaditemmodel.h"
#include "iqiscadmodelrebuilder.h"

#include "astbase.h"
#include "base/exception.h"
#include "base/exceptionhandling.h"




IQISCADScriptJob::IQISCADScriptJob(
    const std::string& script,
    Task executeUntilTask,
    QObject* parent )
  : script_(script),
    reusesParseResult_(false),
    finalTask_(executeUntilTask),
    cancelled_(false),
    threadActive_(false),
    rebuildInterrupted_(false),
    workerRunning_(false)
{
    setParent(parent);

    // queued, since emitted from the background thread
    connect(this, &IQISCADScriptJob::threadEnded,
            this, &IQISCADScriptJob::onThreadEnded );
}




IQISCADScriptJob::IQISCADScriptJob(
    ISCADParseResultPtr parseResult,
    Task executeUntilTask,
    QObject* parent )
  : IQISCADScriptJob(parseResult->script, executeUntilTask, parent)
{
    parseResult_=parseResult;
    reusesParseResult_=true;
}




IQISCADScriptJob::~IQISCADScriptJob()
{
    stopAndWait();
}




const std::string& IQISCADScriptJob::script() const
{
    return script_;
}




IQISCADScriptModelGenerator::Task IQISCADScriptJob::finalTask() const
{
    return finalTask_;
}




bool IQISCADScriptJob::reusesParseResult() const
{
    return reusesParseResult_;
}




ISCADParseResultPtr IQISCADScriptJob::parseResult() const
{
    return parseResult_;
}




void IQISCADScriptJob::launch(IQCADItemModel* model)
{
    insight::CurrentExceptionContext ex("launching ISCAD script job");

    if (cancelled_)
    {
        // cancelled before launch: no thread involved,
        // but report asynchronously like with thread
        QMetaObject::invokeMethod(
            this,
            [this]() { Q_EMIT threadEnded(); },
            Qt::QueuedConnection );
        return;
    }

    insight::assertion(
        !thread_,
        "internal error: a script job can only be launched once" );

    if (model && finalTask_>=Rebuild)
    {
        // created in GUI thread: the created* signals get queued into the GUI thread
        rb_=std::make_unique<IQISCADModelRebuilder>(
            model, QList<IQISCADModelGenerator*>{this} );
    }

    threadActive_=true;
    thread_=std::make_unique<std::thread>( [this]() { run(); } );
}




void IQISCADScriptJob::run()
{
    auto id = std::this_thread::get_id();
    {
        std::lock_guard<std::mutex> l(workerMtx_);
        workerId_=id;
        workerRunning_=true;
    }
    // remove stale request of some earlier thread with same id
    insight::cad::ASTBase::clearCancelRequest(id);
    if (cancelled_)
    {
        // cancelled before thread id was known
        insight::cad::ASTBase::cancelRebuild(id);
    }

    try
    {
        if (!cancelled_)
        {
            if (!parseResult_)
            {
                parseResult_=parse(script_);
            }

            if (!cancelled_)
            {
                Q_EMIT parsed(parseResult_);

                rebuild(*parseResult_, finalTask_);

                if (!cancelled_ && finalTask_>=Rebuild)
                {
                    Q_EMIT modelRebuilt();
                }
            }
        }
    }
    catch (const insight::cad::RebuildCancelException&)
    {
        rebuildInterrupted_=true;
    }
    catch (...)
    {
        if (finalTask_>=Rebuild)
        {
            // an exception within a feature build might leave it in an undefined state
            rebuildInterrupted_=true;
        }
        if (!cancelled_)
        {
            auto desc = insight::describeCurrentException();
            Q_EMIT scriptError(-1, QString::fromStdString(std::string(*desc)), 0);
        }
    }

    {
        std::lock_guard<std::mutex> l(workerMtx_);
        workerRunning_=false;
    }
    insight::cad::ASTBase::clearCancelRequest(id);

    threadActive_=false;
    Q_EMIT threadEnded();
}




void IQISCADScriptJob::onThreadEnded()
{
    // executed in GUI thread, after all entities emitted by the thread have been
    // delivered to the rebuilder (queued events are processed in order)
    if (thread_)
    {
        thread_->join(); // thread is about to return
        thread_.reset();
    }

    if (rb_)
    {
        if (cancelled_ || rebuildInterrupted_) rb_->abandon();
        rb_.reset(); // removes symbols, which were not recreated
    }
}




void IQISCADScriptJob::cancel()
{
    cancelled_=true;

    {
        std::lock_guard<std::mutex> l(workerMtx_);
        if (workerRunning_)
        {
            insight::cad::ASTBase::cancelRebuild(workerId_);
        }
    }

    if (rb_)
    {
        // discard all entities of this job,
        // including those, which are already queued
        rb_->abandon();
        rb_.reset();
    }
}




void IQISCADScriptJob::cancelAndDeleteLater()
{
    cancel();

    // connect before checking: the thread might end in between.
    // Calling deleteLater more than once is safe.
    connect(this, &IQISCADScriptJob::threadEnded,
            this, &QObject::deleteLater );

    if (!threadActive_)
    {
        deleteLater();
    }
}




void IQISCADScriptJob::stopAndWait()
{
    cancel();

    if (thread_)
    {
        thread_->join();
        thread_.reset();
    }
}




bool IQISCADScriptJob::isRunning() const
{
    return threadActive_;
}




bool IQISCADScriptJob::isCancelled() const
{
    return cancelled_;
}




bool IQISCADScriptJob::rebuildInterrupted() const
{
    return rebuildInterrupted_;
}
