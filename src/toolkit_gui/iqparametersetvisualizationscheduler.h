#ifndef IQPARAMETERSETVISUALIZATIONSCHEDULER_H
#define IQPARAMETERSETVISUALIZATIONSCHEDULER_H

#include "toolkit_gui_export.h"

#include <functional>
#include <exception>

#include <QObject>
#include <QPointer>

#include "iqdebouncedjobscheduler.h"

#include "base/supplementedinputdata.h"


class IQCADItemModel;

namespace insight
{

class CADParameterSetVisualizerGenerator;




/**
 * @brief The IQParameterSetVisualizationScheduler class
 * manages the (re-)computation of a parameter set visualization upon parameter changes.
 *
 * - changes are collected during a debounce interval before a computation is launched
 * - a running computation is cancelled without blocking the GUI thread,
 *   its output is discarded
 * - a new computation is launched only after the cancelled one has actually ended,
 *   i.e. at most one visualization thread is active at any time
 * - only the signals of the current computation are forwarded
 *
 * The scheduling logic is implemented in IQDebouncedJobScheduler.
 *
 * Lives in and must only be used from the GUI thread.
 */
class TOOLKIT_GUI_EXPORT IQParameterSetVisualizationScheduler
    : public IQDebouncedJobScheduler
{
    Q_OBJECT

public:
    typedef std::function<CADParameterSetVisualizerGenerator*(QObject* parent)>
        VisualizerFactory;

private:
    VisualizerFactory factory_;
    QPointer<IQCADItemModel> model_;

    /**
     * @brief currentSid_
     * supplemented input data of the current computation.
     * Reset upon each parameter change.
     */
    supplementedInputDataBasePtr currentSid_;

protected:
    QObject* createDebouncedJob(quint64 id) override;
    void startJob(QObject* job) override;
    void cancelJobAndDeleteLater(QObject* job) override;
    void stopJobAndWait(QObject* job) override;
    void jobLaunchFailed(std::exception_ptr ex) override;
    void currentJobInvalidated() override;

public:
    IQParameterSetVisualizationScheduler(
        VisualizerFactory factory,
        IQCADItemModel* model,
        int debounceMilliseconds = 100,
        QObject* parent = nullptr );

    /**
     * cancels all computations and waits for their threads to end
     */
    ~IQParameterSetVisualizationScheduler();

    CADParameterSetVisualizerGenerator* currentVisualizer() const;

    /**
     * @brief upToDateSupplementedInputData
     * @return
     * the supplemented input data, which was computed from the current parameters,
     * if the scheduler is idle. Null, if a computation is pending/running
     * or the supplemented input data could not be computed completely.
     */
    supplementedInputDataBasePtr upToDateSupplementedInputData() const;

Q_SIGNALS:
    void visualizationCalculationFinished(bool success);
    void updateSupplementedInputData(insight::supplementedInputDataBasePtr sid);
    void visualizationComputationError(std::exception_ptr ex);

    /**
     * @brief inputDataPending
     * the parameters have changed: the previously reported issues are obsolete,
     * new input data is going to be computed.
     */
    void inputDataPending();

    /**
     * @brief inputDataIssuesChanged
     * the warnings and errors of the input data computed from the current parameters
     */
    void inputDataIssuesChanged(insight::InputDataIssueList issues);
};




} // namespace insight

#endif // IQPARAMETERSETVISUALIZATIONSCHEDULER_H
