#ifndef IQPARAMETERSETMODELOBSERVER_H
#define IQPARAMETERSETMODELOBSERVER_H

#include "toolkit_gui_export.h"

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <string>
#include <vector>

#include "iqparametersetmodel.h"


class IQParameter;


/**
 * @brief findParameter
 * resolve a parameter path in the parameter set of the model.
 * @return
 * null, if the path does not exist (e.g. because it is located inside
 * an inactive branch of a selectable subset)
 */
TOOLKIT_GUI_EXPORT const insight::Parameter* findParameter(
    const IQParameterSetModel* model,
    const std::string& path );

/**
 * @brief ensureParameterWrapper
 * Make sure, that the GUI wrapper (IQParameter) for the parameter at path exists.
 * Wrappers are created lazily by the model and only parameters with a wrapper
 * forward their change signals to the model (and thus to all views and the visualization).
 * Has to be called before any parameter is modified directly.
 * @return
 * the wrapper or null, if the path does not exist
 */
TOOLKIT_GUI_EXPORT IQParameter* ensureParameterWrapper(
    IQParameterSetModel* model,
    const std::string& path );

/**
 * @brief modifyParameter
 * resolve parameter at path, ensure that its wrapper exists and
 * apply the modification function to it.
 * Exceptions are reported in a message box.
 * @return
 * true, if the parameter was found and the modification succeeded
 */
TOOLKIT_GUI_EXPORT bool modifyParameter(
    IQParameterSetModel* model,
    const std::string& path,
    std::function<void(insight::Parameter&)> modification,
    QWidget* messageParent = nullptr );

template<class P>
bool modifyParameterAs(
    IQParameterSetModel* model,
    const std::string& path,
    std::function<void(P&)> modification,
    QWidget* messageParent = nullptr )
{
    return modifyParameter(
        model, path,
        [&modification](insight::Parameter& p)
        {
            modification(dynamic_cast<P&>(p));
        },
        messageParent );
}

/**
 * @brief joinParameterPath
 * concatenate path components, ignoring empty ones
 */
TOOLKIT_GUI_EXPORT std::string joinParameterPath(
    const std::string& base, const std::string& rel );




/**
 * @brief The IQParameterSetModelObserver class
 * Watches a parameter set model for any change
 * (value changes, insertions, removals, resets, bulk updates)
 * and emits a single, deferred parameterSetChanged() signal per burst of changes.
 *
 * This is the basic building block for keeping custom GUIs (like wizards)
 * in sync with a parameter set, which is concurrently edited elsewhere
 * (parameter tree, 3D view).
 *
 * Observers should never cache pointers to parameters,
 * since they become invalid on model reset.
 * Instead, they should resolve parameter paths on each notification.
 */
class TOOLKIT_GUI_EXPORT IQParameterSetModelObserver
    : public QObject
{
    Q_OBJECT

    QPointer<IQParameterSetModel> model_;
    std::vector<std::string> watchedPaths_;
    QTimer* timer_;

    bool isRelevant(const QModelIndex& index) const;
    void onSourceChange(const QModelIndex& index);

public:
    IQParameterSetModelObserver(
        IQParameterSetModel* model,
        QObject* parent = nullptr );

    IQParameterSetModel* model() const;

    /**
     * @brief setWatchedPaths
     * restrict notifications to changes of parameters at or below
     * (or containing) one of the given paths.
     * If empty, all changes are notified.
     */
    void setWatchedPaths(const std::vector<std::string>& paths);

public Q_SLOTS:
    /**
     * @brief scheduleNotification
     * emit parameterSetChanged(), once control returns to the event loop
     */
    void scheduleNotification();

Q_SIGNALS:
    void parameterSetChanged();
};


#endif // IQPARAMETERSETMODELOBSERVER_H
