#include "iqiscadscriptjobscheduler.h"

#include "iqcaditemmodel.h"

#include "base/exception.h"
#include "base/exceptionhandling.h"




IQISCADScriptJobScheduler::IQISCADScriptJobScheduler(
    ScriptProvider currentScript,
    IQCADItemModel* model,
    int debounceMilliseconds,
    QObject* parent )
  : insight::IQDebouncedJobScheduler(debounceMilliseconds, parent),
    currentScript_(currentScript),
    model_(model),
    backgroundParsing_(true)
{}




IQISCADScriptJobScheduler::~IQISCADScriptJobScheduler()
{
    // the jobs are children and still complete at this point
    shutdown();
}




void IQISCADScriptJobScheduler::setModel(IQCADItemModel* model)
{
    model_=model;
}




void IQISCADScriptJobScheduler::setBackgroundParsingEnabled(bool enabled)
{
    if (enabled!=backgroundParsing_)
    {
        backgroundParsing_=enabled;
        if (backgroundParsing_)
        {
            requestUpdate();
        }
        else
        {
            auto *job = currentScriptJob();
            if (job && isCurrentJobRunning() && job->finalTask()<IQISCADScriptModelGenerator::Rebuild)
            {
                cancelCurrent();
            }
        }
    }
}




bool IQISCADScriptJobScheduler::backgroundParsingEnabled() const
{
    return backgroundParsing_;
}




ISCADParseResultPtr IQISCADScriptJobScheduler::lastParseResult() const
{
    return lastParse_;
}




void IQISCADScriptJobScheduler::discardParseResult()
{
    lastParse_.reset();
}




IQISCADScriptJob* IQISCADScriptJobScheduler::currentScriptJob() const
{
    return static_cast<IQISCADScriptJob*>(currentJob());
}




IQISCADScriptJob* IQISCADScriptJobScheduler::setupJob(quint64 id, IQISCADScriptJob* job)
{
    auto task = job->finalTask();

    // only forward signals of the current job
    connect(
        job, &IQISCADScriptJob::parsed,
        this, [this,id,task](ISCADParseResultPtr pr)
        {
            if (isCurrent(id))
            {
                lastParse_=pr;
                Q_EMIT parsed(pr, task);
            }
        });
    connect(
        job, &IQISCADScriptJob::scriptError,
        this, [this,id,task](long failpos, QString errorMsg, int range)
        {
            if (isCurrent(id))
                Q_EMIT scriptError(failpos, errorMsg, range, task);
        });
    connect(
        job, &IQISCADScriptJob::statusMessage,
        this, [this,id](const QString& msg, double timeout)
        {
            if (isCurrent(id))
                Q_EMIT statusMessage(msg, timeout);
        });
    connect(
        job, &IQISCADScriptJob::statusProgress,
        this, [this,id](int step, int totalSteps)
        {
            if (isCurrent(id))
                Q_EMIT statusProgress(step, totalSteps);
        });
    connect(
        job, &IQISCADScriptJob::modelRebuilt,
        this, [this,id]()
        {
            if (isCurrent(id))
                Q_EMIT modelRebuilt();
        });

    // connected after the jobs internal handler:
    // is delivered after the output of the job has been processed
    QPointer<IQISCADScriptJob> jp(job);
    connect(
        job, &IQISCADScriptJob::threadEnded,
        this, [this,id,jp,task]()
        {
            if (jp && jp->rebuildInterrupted()
                && lastParse_ && lastParse_==jp->parseResult())
            {
                // the model might contain partially built entities: don't reuse
                lastParse_.reset();
            }

            if (isCurrent(id))
                Q_EMIT jobEnded(task);

            jobThreadEnded(id);
        });

    return job;
}




QObject* IQISCADScriptJobScheduler::createDebouncedJob(quint64 id)
{
    if (!backgroundParsing_)
        return nullptr;

    return setupJob(
        id,
        new IQISCADScriptJob(
            currentScript_(),
            IQISCADScriptModelGenerator::Parse,
            this ) );
}




void IQISCADScriptJobScheduler::startJob(QObject* job)
{
    auto *sj = static_cast<IQISCADScriptJob*>(job);
    Q_EMIT jobStarted(sj->finalTask(), sj->reusesParseResult());
    sj->launch(model_);
}




void IQISCADScriptJobScheduler::cancelJobAndDeleteLater(QObject* job)
{
    static_cast<IQISCADScriptJob*>(job)->cancelAndDeleteLater();
}




void IQISCADScriptJobScheduler::stopJobAndWait(QObject* job)
{
    static_cast<IQISCADScriptJob*>(job)->stopAndWait();
}




void IQISCADScriptJobScheduler::jobLaunchFailed(std::exception_ptr ex)
{
    try
    {
        std::rethrow_exception(ex);
    }
    catch (...)
    {
        auto desc = insight::describeCurrentException();
        Q_EMIT scriptError(
            -1, QString::fromStdString(std::string(*desc)), 0,
            IQISCADScriptModelGenerator::Rebuild );
    }
}




void IQISCADScriptJobScheduler::rebuild(Task executeUntilTask)
{
    auto *job = currentScriptJob();
    if (job && isCurrentJobRunning() && job->finalTask()>=IQISCADScriptModelGenerator::Rebuild)
    {
        // replaced by the new rebuild
        cancelCurrent();
    }

    auto launchRebuild = [this,executeUntilTask]()
    {
        auto script = currentScript_();
        if (lastParse_ && lastParse_->script==script)
        {
            auto pr = lastParse_;
            launchNow(
                [this,pr,executeUntilTask](quint64 id)
                {
                    return setupJob(id, new IQISCADScriptJob(pr, executeUntilTask, this));
                } );
        }
        else
        {
            launchNow(
                [this,script,executeUntilTask](quint64 id)
                {
                    return setupJob(id, new IQISCADScriptJob(script, executeUntilTask, this));
                } );
        }
    };

    if (backgroundParsing_)
    {
        if (!isIdle())
        {
            Q_EMIT waitingForBackgroundParse();
        }
        // re-reads the script, once the background parse of the current script has finished
        whenIdle(launchRebuild);
    }
    else
    {
        cancelWhenIdle();
        launchRebuild();
    }
}




void IQISCADScriptJobScheduler::rebuildScript(const std::string& script, Task executeUntilTask)
{
    cancelWhenIdle();
    launchNow(
        [this,script,executeUntilTask](quint64 id)
        {
            return setupJob(id, new IQISCADScriptJob(script, executeUntilTask, this));
        } );
}




void IQISCADScriptJobScheduler::stop()
{
    cancelWhenIdle();
    cancel();
}
