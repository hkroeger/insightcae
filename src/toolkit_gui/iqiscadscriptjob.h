#ifndef IQISCADSCRIPTJOB_H
#define IQISCADSCRIPTJOB_H

#include "toolkit_gui_export.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "iqiscadscriptmodelgenerator.h"


class IQCADItemModel;
class IQISCADModelRebuilder;




/**
 * @brief The IQISCADScriptJob class
 * parses and/or rebuilds an ISCAD model script in a background thread.
 *
 * Either parses the given script or reuses a given parse result.
 * The rebuilt entities are inserted into a CAD item model.
 *
 * Must be created and controlled from the GUI thread.
 * Can be launched only once.
 */
class TOOLKIT_GUI_EXPORT IQISCADScriptJob
    : public IQISCADScriptModelGenerator
{
    Q_OBJECT

    std::string script_;
    ISCADParseResultPtr parseResult_;
    bool reusesParseResult_;
    Task finalTask_;

    std::unique_ptr<IQISCADModelRebuilder> rb_;
    std::unique_ptr<std::thread> thread_;

    std::atomic<bool> cancelled_;
    std::atomic<bool> threadActive_;
    std::atomic<bool> rebuildInterrupted_;

    std::mutex workerMtx_;
    std::thread::id workerId_;
    bool workerRunning_;

    void run();
    void onThreadEnded();

public:
    /**
     * @brief IQISCADScriptJob
     * parse the script and execute the given task
     */
    IQISCADScriptJob(
        const std::string& script,
        Task executeUntilTask,
        QObject* parent = nullptr );

    /**
     * @brief IQISCADScriptJob
     * reuse the given parse result and execute the given task
     */
    IQISCADScriptJob(
        ISCADParseResultPtr parseResult,
        Task executeUntilTask,
        QObject* parent = nullptr );

    ~IQISCADScriptJob();

    const std::string& script() const;
    Task finalTask() const;
    bool reusesParseResult() const;

    /**
     * @brief parseResult
     * the parse result used by this job.
     * Available after the parsed signal was emitted.
     */
    ISCADParseResultPtr parseResult() const;

    /**
     * @brief launch
     * start the background thread.
     * @param model
     * the CAD model, into which the rebuilt entities are inserted.
     * Only used, if the task includes a rebuild.
     */
    void launch(IQCADItemModel* model);

    /**
     * @brief cancel
     * request cancellation and return immediately.
     * All remaining output is discarded, the entities of a partially rebuilt model
     * are left untouched.
     * threadEnded() is emitted, once the thread has actually ended.
     */
    void cancel();

    /**
     * @brief cancelAndDeleteLater
     * cancel without blocking and delete this object, once the background thread has ended
     */
    void cancelAndDeleteLater();

    /**
     * @brief stopAndWait
     * cancel and block until the background thread has ended.
     */
    void stopAndWait();

    bool isRunning() const;
    bool isCancelled() const;

    /**
     * @brief rebuildInterrupted
     * true, if the rebuild was interrupted.
     * The model of the parse result may then contain partially built entities.
     */
    bool rebuildInterrupted() const;

Q_SIGNALS:
    /**
     * @brief parsed
     * the script was parsed (or the given parse result is going to be reused)
     */
    void parsed(ISCADParseResultPtr parseResult);

    /**
     * @brief modelRebuilt
     * the rebuild task was executed completely
     */
    void modelRebuilt();

    /**
     * @brief threadEnded
     * emitted in any case (success, error, cancellation),
     * when the background thread has ended
     */
    void threadEnded();
};




#endif // IQISCADSCRIPTJOB_H
