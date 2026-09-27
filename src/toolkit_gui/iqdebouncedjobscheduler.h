#ifndef IQDEBOUNCEDJOBSCHEDULER_H
#define IQDEBOUNCEDJOBSCHEDULER_H

#include "toolkit_gui_export.h"

#include <functional>
#include <exception>

#include <QObject>
#include <QPointer>
#include <QTimer>


namespace insight
{




/**
 * @brief The IQDebouncedJobScheduler class
 * manages the (re-)execution of a background job upon changes of its input.
 *
 * - changes are collected during a debounce interval before a job is launched
 * - a running job is cancelled without blocking the GUI thread,
 *   its output is discarded
 * - a new job is launched only after the cancelled one has actually ended,
 *   i.e. at most one job thread is active at any time
 * - only the signals of the current job are to be forwarded
 *   (derived classes filter by isCurrent(id))
 * - besides the debounced jobs, jobs can be launched explicitly (launchNow).
 *   Jobs can be protected against cancellation by input changes.
 *
 * A job is a QObject, which runs a computation in a background thread.
 * The derived class implements creation, start and cancellation of the jobs
 * and has to call jobThreadEnded(id), once the thread of job id has ended.
 *
 * Derived classes have to call shutdown() in their destructor.
 *
 * Lives in and must only be used from the GUI thread.
 */
class TOOLKIT_GUI_EXPORT IQDebouncedJobScheduler
    : public QObject
{
    Q_OBJECT

public:
    typedef std::function<QObject*(quint64 id)> JobFactory;

private:
    QTimer debounceTimer_;

    /**
     * @brief current_
     * the most recently launched job. Is kept after it has ended.
     */
    QPointer<QObject> current_;
    quint64 currentId_;
    bool currentEnded_;
    bool currentCancelOnUpdate_;
    bool currentDebounced_;

    /**
     * @brief retiring_
     * a cancelled job, whose thread has not yet ended
     */
    QPointer<QObject> retiring_;
    quint64 retiringId_;

    /**
     * @brief pendingDebouncedLaunch_
     * the debounce interval has passed, but the launch has to wait
     * for the end of another job
     */
    bool pendingDebouncedLaunch_;

    /**
     * @brief pendingExplicitLaunch_
     * explicitly requested job, which waits for the end of the cancelled job
     */
    JobFactory pendingExplicitLaunch_;
    bool pendingExplicitCancelOnUpdate_;

    std::function<void()> idleCallback_;

    quint64 lastId_;
    int launchedCount_;

    void onDebounceTimeout();
    bool isCurrentRunning() const;
    void retireCurrent();
    void launchPendingOrCheckIdle();
    void launchJob(JobFactory factory, bool cancelOnUpdate, bool debounced);
    void checkIdle();

protected:
    /**
     * @brief createDebouncedJob
     * create a job for the current input. Connect its signals
     * (forward only if isCurrent(id)) and connect its end to jobThreadEnded(id).
     * @return
     * the new job or null, if there is nothing to do
     */
    virtual QObject* createDebouncedJob(quint64 id) =0;

    /**
     * @brief startJob
     * start the computation of the job. May throw.
     */
    virtual void startJob(QObject* job) =0;

    /**
     * @brief cancelJobAndDeleteLater
     * cancel the job without blocking and delete it, once its thread has ended.
     * jobThreadEnded has still to be called for this job.
     */
    virtual void cancelJobAndDeleteLater(QObject* job) =0;

    /**
     * @brief stopJobAndWait
     * cancel the job and block until its thread has ended
     */
    virtual void stopJobAndWait(QObject* job) =0;

    /**
     * @brief jobLaunchFailed
     * called, if startJob throws
     */
    virtual void jobLaunchFailed(std::exception_ptr ex);

    /**
     * @brief currentJobInvalidated
     * called, whenever the result of the current job becomes obsolete
     * (input changes, cancellation, launch of a new job)
     */
    virtual void currentJobInvalidated();

    /**
     * @brief jobThreadEnded
     * has to be called (in GUI thread), once the thread of the job with the given id has ended
     * and its output has been delivered.
     */
    void jobThreadEnded(quint64 id);

    bool isCurrent(quint64 id) const;

    QObject* currentJob() const;

    bool isCurrentJobRunning() const;

    /**
     * @brief shutdown
     * cancel all jobs and wait for their threads to end.
     * To be called from the destructor of the derived class.
     */
    void shutdown();

public:
    IQDebouncedJobScheduler(
        int debounceMilliseconds = 100,
        QObject* parent = nullptr );

    ~IQDebouncedJobScheduler();

    /**
     * @brief isIdle
     * true, if there is nothing scheduled or running
     * and the output of the last job has been completely delivered
     */
    bool isIdle() const;

    /**
     * @brief launchedCount
     * number of jobs launched so far
     */
    int launchedCount() const;

    /**
     * @brief whenIdle
     * call f, once the job for the current input has completed
     * (immediately, if idle already). A pending debounce interval is skipped.
     * Replaces any previously registered callback.
     */
    void whenIdle(std::function<void()> f);

    /**
     * @brief cancelWhenIdle
     * drop the callback registered by whenIdle
     */
    void cancelWhenIdle();

    /**
     * @brief launchNow
     * launch a job explicitly, without debounce interval.
     * A running job is cancelled, the new job is started once it has ended.
     * Takes precedence over a pending debounced launch.
     * If the cancelled job was a debounced job, it is relaunched afterwards.
     * @param factory
     * creates the job, like createDebouncedJob
     * @param cancelOnUpdate
     * if false, the job is not cancelled by requestUpdate
     * (the next debounced job is launched after it has ended).
     */
    void launchNow(JobFactory factory, bool cancelOnUpdate = false);

public Q_SLOTS:
    /**
     * @brief requestUpdate
     * to be called upon each input change
     */
    void requestUpdate();

    /**
     * @brief cancel
     * cancel any scheduled or running job (non-blocking)
     */
    void cancel();

    /**
     * @brief cancelCurrent
     * cancel the running job only (non-blocking).
     * Scheduled launches are kept.
     */
    void cancelCurrent();

    /**
     * @brief flush
     * launch a pending job immediately, skipping the rest of the debounce interval
     */
    void flush();

Q_SIGNALS:
    void becameIdle();
};




} // namespace insight

#endif // IQDEBOUNCEDJOBSCHEDULER_H
