#ifndef IQPARAMETERSETVISUALIZATIONSCHEDULER_H
#define IQPARAMETERSETVISUALIZATIONSCHEDULER_H

#include "toolkit_gui_export.h"

#include <functional>
#include <exception>

#include <QObject>
#include <QPointer>
#include <QTimer>

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
 * Lives in and must only be used from the GUI thread.
 */
class TOOLKIT_GUI_EXPORT IQParameterSetVisualizationScheduler
    : public QObject
{
    Q_OBJECT

public:
    typedef std::function<CADParameterSetVisualizerGenerator*(QObject* parent)>
        VisualizerFactory;

private:
    VisualizerFactory factory_;
    QPointer<IQCADItemModel> model_;

    QTimer debounceTimer_;

    /**
     * @brief current_
     * the most recently launched visualizer. Is kept after its computation has ended.
     */
    QPointer<CADParameterSetVisualizerGenerator> current_;
    quint64 currentId_;
    bool currentEnded_;

    /**
     * @brief retiring_
     * a cancelled visualizer, whose thread has not yet ended
     */
    QPointer<CADParameterSetVisualizerGenerator> retiring_;
    quint64 retiringId_;

    bool pendingLaunch_;

    /**
     * @brief currentSid_
     * supplemented input data of the current computation.
     * Reset upon each parameter change.
     */
    supplementedInputDataBasePtr currentSid_;

    std::function<void()> idleCallback_;

    quint64 lastId_;
    int launchedCount_;

    void onDebounceTimeout();
    void onVisualizerThreadEnded(quint64 id);
    void launchNew();
    void checkIdle();

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
     * @brief isIdle
     * true, if there is nothing scheduled or running
     * and the output of the last computation has been completely delivered
     */
    bool isIdle() const;

    /**
     * @brief launchedCount
     * number of computations launched so far
     */
    int launchedCount() const;

    /**
     * @brief upToDateSupplementedInputData
     * @return
     * the supplemented input data, which was computed from the current parameters,
     * if the scheduler is idle. Null, if a computation is pending/running
     * or no supplemented input data could be computed.
     */
    supplementedInputDataBasePtr upToDateSupplementedInputData() const;

    /**
     * @brief whenIdle
     * call f, once the visualization of the current parameters has completed
     * (immediately, if idle already). A pending debounce interval is skipped.
     * Replaces any previously registered callback.
     */
    void whenIdle(std::function<void()> f);

    /**
     * @brief cancelWhenIdle
     * drop the callback registered by whenIdle
     */
    void cancelWhenIdle();

public Q_SLOTS:
    /**
     * @brief requestUpdate
     * to be called upon each parameter change
     */
    void requestUpdate();

    /**
     * @brief cancel
     * cancel any scheduled or running computation (non-blocking)
     */
    void cancel();

    /**
     * @brief flush
     * launch a pending computation immediately, skipping the rest of the debounce interval
     */
    void flush();

Q_SIGNALS:
    void becameIdle();

    void visualizationCalculationFinished(bool success);
    void updateSupplementedInputData(insight::supplementedInputDataBasePtr sid);
    void visualizationComputationError(std::exception_ptr ex);
};




} // namespace insight

#endif // IQPARAMETERSETVISUALIZATIONSCHEDULER_H
