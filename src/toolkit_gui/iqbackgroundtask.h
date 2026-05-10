#ifndef IQBACKGROUNDTASK_H
#define IQBACKGROUNDTASK_H

#include <QObject>
#include "base/insightthread.h"
#include "base/progressdisplayer.h"


class IQBackgroundTaskInterface
: public QObject
{
    Q_OBJECT

Q_SIGNALS:
    /**
   * @brief finished
   * this is only emitted, if action is successfully completed without exceptions
   */
    void finished();

    /**
   * @brief failed
   * the action failed with an exception
   * @param exception
   * the exception, which occurred
   */
    void failed(::std::exception_ptr exception);

    /**
   * @brief cancelled
   * the action was interrupted
   */
    void cancelled();
};




class IQBackgroundTask
    : public IQBackgroundTaskInterface,
      public insight::Thread
{
    std::string name_;
    insight::ProgressDisplayer* pd_;

public:
    IQBackgroundTask(
        const std::string& name,
        insight::ProgressDisplayer *pd=nullptr );

    /**
     * @brief start
     * start the task.
     * is in extra function, because the signals have to be connected before the actual task starts.
     * @param action
     */
    void start(
        std::function<void(insight::ActionProgress&)> action,
        bool installDefaultFailureSignalReceiver = true
    );
};

#endif //
