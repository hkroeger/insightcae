#include "iqparametersetvisualizationscheduler.h"

#include "cadparametersetvisualizer.h"
#include "iqcaditemmodel.h"

#include "base/exception.h"
#include "base/exceptionhandling.h"

namespace insight
{




IQParameterSetVisualizationScheduler::IQParameterSetVisualizationScheduler(
    VisualizerFactory factory,
    IQCADItemModel* model,
    int debounceMilliseconds,
    QObject* parent )
  : QObject(parent),
    factory_(factory),
    model_(model),
    currentId_(0),
    currentEnded_(true),
    retiringId_(0),
    pendingLaunch_(false),
    lastId_(0),
    launchedCount_(0)
{
    debounceTimer_.setSingleShot(true);
    debounceTimer_.setInterval(debounceMilliseconds);
    connect(&debounceTimer_, &QTimer::timeout,
            this, &IQParameterSetVisualizationScheduler::onDebounceTimeout );
}




IQParameterSetVisualizationScheduler::~IQParameterSetVisualizationScheduler()
{
    debounceTimer_.stop();

    // the visualizers are children and still complete at this point
    if (retiring_)
        retiring_->stopAndWait();
    if (current_)
        current_->stopAndWait();
}




CADParameterSetVisualizerGenerator*
IQParameterSetVisualizationScheduler::currentVisualizer() const
{
    return current_;
}




bool IQParameterSetVisualizationScheduler::isIdle() const
{
    return
        !debounceTimer_.isActive()
        && !pendingLaunch_
        && retiringId_==0
        && currentEnded_;
}




int IQParameterSetVisualizationScheduler::launchedCount() const
{
    return launchedCount_;
}




supplementedInputDataBasePtr
IQParameterSetVisualizationScheduler::upToDateSupplementedInputData() const
{
    if (isIdle())
        return currentSid_;
    return nullptr;
}




void IQParameterSetVisualizationScheduler::whenIdle(std::function<void()> f)
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




void IQParameterSetVisualizationScheduler::cancelWhenIdle()
{
    idleCallback_ = nullptr;
}




void IQParameterSetVisualizationScheduler::flush()
{
    if (debounceTimer_.isActive())
    {
        debounceTimer_.stop();
        onDebounceTimeout();
    }
}




void IQParameterSetVisualizationScheduler::checkIdle()
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




void IQParameterSetVisualizationScheduler::cancel()
{
    debounceTimer_.stop();
    pendingLaunch_=false;
    currentSid_.reset();

    if (current_ && !currentEnded_)
    {
        insight::assertion(
            retiringId_==0,
            "internal error: a computation is running while another has not yet ended" );

        retiring_=current_;
        retiringId_=currentId_;
        current_=nullptr;
        currentId_=0;
        currentEnded_=true;

        retiring_->cancelAndDeleteLater();
    }
}




void IQParameterSetVisualizationScheduler::requestUpdate()
{
    cancel();
    debounceTimer_.start();
}




void IQParameterSetVisualizationScheduler::onDebounceTimeout()
{
    if (retiringId_!=0)
    {
        // wait for cancelled computation to end
        pendingLaunch_=true;
    }
    else
    {
        launchNew();
    }
}




void IQParameterSetVisualizationScheduler::onVisualizerThreadEnded(quint64 id)
{
    if (id==currentId_)
    {
        currentEnded_=true;
        checkIdle();
    }
    else if (id==retiringId_)
    {
        retiring_=nullptr; // deletes itself
        retiringId_=0;

        if (pendingLaunch_ && !debounceTimer_.isActive())
        {
            launchNew();
        }
        else
        {
            checkIdle();
        }
    }
}




void IQParameterSetVisualizationScheduler::launchNew()
{
    CurrentExceptionContext ex("launching new parameter set visualization");

    pendingLaunch_=false;
    currentSid_.reset();

    if (current_)
    {
        // computation has ended, no need to wait
        current_->deleteLater();
        current_=nullptr;
        currentId_=0;
    }

    auto *viz = factory_(this);
    if (!viz)
    {
        currentEnded_=true;
        checkIdle();
        return;
    }

    quint64 id = ++lastId_;
    current_=viz;
    currentId_=id;
    currentEnded_=false;

    // only forward signals of the current computation
    connect(
        viz, &CADParameterSetVisualizerGenerator::updateSupplementedInputData,
        this, [this,id](insight::supplementedInputDataBasePtr sid)
        {
            if (id==currentId_)
            {
                currentSid_=sid;
                Q_EMIT updateSupplementedInputData(sid);
            }
        });
    connect(
        viz, &CADParameterSetVisualizerGenerator::visualizationCalculationFinished,
        this, [this,id](bool success)
        {
            if (id==currentId_)
                Q_EMIT visualizationCalculationFinished(success);
        });
    connect(
        viz, &CADParameterSetVisualizerGenerator::visualizationComputationError,
        this, [this,id](std::exception_ptr ex)
        {
            if (id==currentId_)
                Q_EMIT visualizationComputationError(ex);
        });

    // connected after the visualizers internal handler:
    // is delivered after the output of the computation has been processed
    connect(
        viz, &CADParameterSetVisualizerGenerator::computationThreadEnded,
        this, [this,id]() { onVisualizerThreadEnded(id); } );

    ++launchedCount_;

    try
    {
        viz->launch(model_);
    }
    catch (...)
    {
        currentEnded_=true;
        Q_EMIT visualizationComputationError(std::current_exception());
        checkIdle();
    }
}




} // namespace insight
