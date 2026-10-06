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
  : IQDebouncedJobScheduler(debounceMilliseconds, parent),
    factory_(factory),
    model_(model)
{}




IQParameterSetVisualizationScheduler::~IQParameterSetVisualizationScheduler()
{
    // the visualizers are children and still complete at this point
    shutdown();
}




CADParameterSetVisualizerGenerator*
IQParameterSetVisualizationScheduler::currentVisualizer() const
{
    return static_cast<CADParameterSetVisualizerGenerator*>(currentJob());
}




supplementedInputDataBasePtr
IQParameterSetVisualizationScheduler::upToDateSupplementedInputData() const
{
    // never hand out incomplete input data (e.g. for starting a run)
    if (isIdle() && currentSid_ && currentSid_->complete())
        return currentSid_;
    return nullptr;
}




QObject* IQParameterSetVisualizationScheduler::createDebouncedJob(quint64 id)
{
    CurrentExceptionContext ex("creating new parameter set visualization");

    auto *viz = factory_(this);
    if (!viz)
        return nullptr;

    // only forward signals of the current computation
    connect(
        viz, &CADParameterSetVisualizerGenerator::updateSupplementedInputData,
        this, [this,id](insight::supplementedInputDataBasePtr sid)
        {
            if (isCurrent(id))
            {
                currentSid_=sid;
                Q_EMIT updateSupplementedInputData(sid);
            }
        });
    connect(
        viz, &CADParameterSetVisualizerGenerator::visualizationCalculationFinished,
        this, [this,id](bool success)
        {
            if (isCurrent(id))
                Q_EMIT visualizationCalculationFinished(success);
        });
    connect(
        viz, &CADParameterSetVisualizerGenerator::visualizationComputationError,
        this, [this,id](std::exception_ptr ex)
        {
            if (isCurrent(id))
                Q_EMIT visualizationComputationError(ex);
        });
    connect(
        viz, &CADParameterSetVisualizerGenerator::inputDataIssuesChanged,
        this, [this,id](insight::InputDataIssueList issues)
        {
            if (isCurrent(id))
                Q_EMIT inputDataIssuesChanged(issues);
        });

    // connected after the visualizers internal handler:
    // is delivered after the output of the computation has been processed
    connect(
        viz, &CADParameterSetVisualizerGenerator::computationThreadEnded,
        this, [this,id]() { jobThreadEnded(id); } );

    return viz;
}




void IQParameterSetVisualizationScheduler::startJob(QObject* job)
{
    static_cast<CADParameterSetVisualizerGenerator*>(job)->launch(model_);
}




void IQParameterSetVisualizationScheduler::cancelJobAndDeleteLater(QObject* job)
{
    static_cast<CADParameterSetVisualizerGenerator*>(job)->cancelAndDeleteLater();
}




void IQParameterSetVisualizationScheduler::stopJobAndWait(QObject* job)
{
    static_cast<CADParameterSetVisualizerGenerator*>(job)->stopAndWait();
}




void IQParameterSetVisualizationScheduler::jobLaunchFailed(std::exception_ptr ex)
{
    Q_EMIT visualizationComputationError(ex);
}




void IQParameterSetVisualizationScheduler::currentJobInvalidated()
{
    currentSid_.reset();
    Q_EMIT inputDataPending();
}




} // namespace insight
