#include "iqdebouncedjobscheduler.h"

#include "base/exception.h"
#include "base/exceptionhandling.h"

namespace insight
{




IQDebouncedJobScheduler::IQDebouncedJobScheduler(
    int debounceMilliseconds,
    QObject* parent )
  : QObject(parent),
    currentId_(0),
    currentEnded_(true),
    currentCancelOnUpdate_(true),
    currentDebounced_(false),
    retiringId_(0),
    pendingDebouncedLaunch_(false),
    pendingExplicitCancelOnUpdate_(false),
    lastId_(0),
    launchedCount_(0)
{
    debounceTimer_.setSingleShot(true);
    debounceTimer_.setInterval(debounceMilliseconds);
    connect(&debounceTimer_, &QTimer::timeout,
            this, &IQDebouncedJobScheduler::onDebounceTimeout );
}




IQDebouncedJobScheduler::~IQDebouncedJobScheduler()
{
    if (retiring_ || (current_ && !currentEnded_))
    {
        std::cerr
            << "Warning: job scheduler deleted while jobs were still active. "
               "Derived classes have to call shutdown() in their destructor."
            << std::endl;
    }
}




void IQDebouncedJobScheduler::shutdown()
{
    debounceTimer_.stop();
    pendingDebouncedLaunch_=false;
    pendingExplicitLaunch_=nullptr;
    idleCallback_=nullptr;

    // the jobs are still complete at this point
    if (retiring_)
        stopJobAndWait(retiring_);
    if (current_)
        stopJobAndWait(current_);

    retiring_=nullptr;
    retiringId_=0;
    currentEnded_=true;
}




void IQDebouncedJobScheduler::jobLaunchFailed(std::exception_ptr)
{}




void IQDebouncedJobScheduler::currentJobInvalidated()
{}




bool IQDebouncedJobScheduler::isCurrent(quint64 id) const
{
    return id!=0 && id==currentId_;
}




QObject* IQDebouncedJobScheduler::currentJob() const
{
    return current_;
}




bool IQDebouncedJobScheduler::isCurrentJobRunning() const
{
    return isCurrentRunning();
}




bool IQDebouncedJobScheduler::isIdle() const
{
    return
        !debounceTimer_.isActive()
        && !pendingDebouncedLaunch_
        && !pendingExplicitLaunch_
        && retiringId_==0
        && currentEnded_;
}




int IQDebouncedJobScheduler::launchedCount() const
{
    return launchedCount_;
}




void IQDebouncedJobScheduler::whenIdle(std::function<void()> f)
{
    if (isIdle())
    {
        idleCallback_ = nullptr;
        f();
    }
    else
    {
        idleCallback_ = f;
        flush();
    }
}




void IQDebouncedJobScheduler::cancelWhenIdle()
{
    idleCallback_ = nullptr;
}




void IQDebouncedJobScheduler::flush()
{
    if (debounceTimer_.isActive())
    {
        debounceTimer_.stop();
        onDebounceTimeout();
    }
}




void IQDebouncedJobScheduler::checkIdle()
{
    if (isIdle())
    {
        Q_EMIT becameIdle();

        if (idleCallback_)
        {
            auto f = idleCallback_;
            idleCallback_ = nullptr;
            f();
        }
    }
}




bool IQDebouncedJobScheduler::isCurrentRunning() const
{
    return current_ && !currentEnded_;
}




void IQDebouncedJobScheduler::retireCurrent()
{
    if (isCurrentRunning())
    {
        insight::assertion(
            retiringId_==0,
            "internal error: a job is running while another has not yet ended" );

        retiring_=current_;
        retiringId_=currentId_;
        current_=nullptr;
        currentId_=0;
        currentEnded_=true;

        cancelJobAndDeleteLater(retiring_);
    }
}




void IQDebouncedJobScheduler::cancel()
{
    debounceTimer_.stop();
    pendingDebouncedLaunch_=false;
    pendingExplicitLaunch_=nullptr;
    currentJobInvalidated();
    retireCurrent();
}




void IQDebouncedJobScheduler::cancelCurrent()
{
    currentJobInvalidated();
    retireCurrent();
}




void IQDebouncedJobScheduler::requestUpdate()
{
    debounceTimer_.stop();
    pendingDebouncedLaunch_=false;
    currentJobInvalidated();

    if (currentCancelOnUpdate_)
    {
        retireCurrent();
    }
    // else: protected job continues,
    // the debounced launch waits for its end

    debounceTimer_.start();
}




void IQDebouncedJobScheduler::launchNow(JobFactory factory, bool cancelOnUpdate)
{
    pendingExplicitLaunch_=factory;
    pendingExplicitCancelOnUpdate_=cancelOnUpdate;

    if (isCurrentRunning() && currentDebounced_)
    {
        // the input has not been processed yet: redo after the explicit job
        pendingDebouncedLaunch_=true;
    }

    currentJobInvalidated();
    retireCurrent();

    launchPendingOrCheckIdle();
}




void IQDebouncedJobScheduler::onDebounceTimeout()
{
    pendingDebouncedLaunch_=true;
    launchPendingOrCheckIdle();
}




void IQDebouncedJobScheduler::jobThreadEnded(quint64 id)
{
    if (id==0)
        return;

    if (id==currentId_)
    {
        currentEnded_=true;
        launchPendingOrCheckIdle();
    }
    else if (id==retiringId_)
    {
        retiring_=nullptr; // deletes itself
        retiringId_=0;
        launchPendingOrCheckIdle();
    }
}




void IQDebouncedJobScheduler::launchPendingOrCheckIdle()
{
    if (retiringId_!=0 || isCurrentRunning())
    {
        // wait for the end of the running job
        return;
    }

    if (pendingExplicitLaunch_)
    {
        auto f = pendingExplicitLaunch_;
        pendingExplicitLaunch_=nullptr;
        launchJob(f, pendingExplicitCancelOnUpdate_, false);
    }
    else if (pendingDebouncedLaunch_ && !debounceTimer_.isActive())
    {
        pendingDebouncedLaunch_=false;
        launchJob(
            [this](quint64 id) { return createDebouncedJob(id); },
            true, true );
    }
    else
    {
        checkIdle();
    }
}




void IQDebouncedJobScheduler::launchJob(JobFactory factory, bool cancelOnUpdate, bool debounced)
{
    CurrentExceptionContext ex("launching new background job");

    currentJobInvalidated();

    if (current_)
    {
        // job has ended, no need to wait
        current_->deleteLater();
        current_=nullptr;
        currentId_=0;
    }

    quint64 id = ++lastId_;
    currentId_=id; // before factory call: the job might check isCurrent(id)
    currentCancelOnUpdate_=true;
    currentDebounced_=false;

    auto *job = factory(id);
    if (!job)
    {
        currentId_=0;
        currentEnded_=true;
        launchPendingOrCheckIdle();
        return;
    }

    current_=job;
    currentEnded_=false;
    currentCancelOnUpdate_=cancelOnUpdate;
    currentDebounced_=debounced;

    ++launchedCount_;

    try
    {
        startJob(job);
    }
    catch (...)
    {
        currentEnded_=true;
        jobLaunchFailed(std::current_exception());
        launchPendingOrCheckIdle();
    }
}




} // namespace insight
