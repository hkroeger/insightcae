#ifndef IQISCADSCRIPTJOBSCHEDULER_H
#define IQISCADSCRIPTJOBSCHEDULER_H

#include "toolkit_gui_export.h"

#include <functional>

#include <QPointer>

#include "iqdebouncedjobscheduler.h"
#include "iqiscadscriptjob.h"


class IQCADItemModel;




/**
 * @brief The IQISCADScriptJobScheduler class
 * manages background parsing and explicit rebuilds of an ISCAD model script.
 *
 * - upon each script change (requestUpdate), a running background parse is cancelled
 *   and a new one is launched after a debounce interval
 * - an explicit rebuild reuses the result of the background parse,
 *   if it matches the current script (waits for the background parse to finish, if required)
 * - explicit rebuilds are not cancelled by script changes,
 *   the next background parse is launched after the rebuild has ended
 * - at most one parser/rebuild thread is active at any time,
 *   only the signals of the current job are forwarded
 *
 * Lives in and must only be used from the GUI thread.
 */
class TOOLKIT_GUI_EXPORT IQISCADScriptJobScheduler
    : public insight::IQDebouncedJobScheduler
{
    Q_OBJECT

public:
    typedef std::function<std::string()> ScriptProvider;
    typedef IQISCADScriptModelGenerator::Task Task;

private:
    ScriptProvider currentScript_;
    QPointer<IQCADItemModel> model_;
    bool backgroundParsing_;

    /**
     * @brief lastParse_
     * the result of the most recent parse
     */
    ISCADParseResultPtr lastParse_;

    IQISCADScriptJob* setupJob(quint64 id, IQISCADScriptJob* job);
    IQISCADScriptJob* currentScriptJob() const;

protected:
    QObject* createDebouncedJob(quint64 id) override;
    void startJob(QObject* job) override;
    void cancelJobAndDeleteLater(QObject* job) override;
    void stopJobAndWait(QObject* job) override;
    void jobLaunchFailed(std::exception_ptr ex) override;

public:
    /**
     * @param currentScript
     * delivers the current script contents
     * @param model
     * the model, into which rebuilt entities are inserted
     */
    IQISCADScriptJobScheduler(
        ScriptProvider currentScript,
        IQCADItemModel* model = nullptr,
        int debounceMilliseconds = 1000,
        QObject* parent = nullptr );

    /**
     * cancels all jobs and waits for their threads to end
     */
    ~IQISCADScriptJobScheduler();

    void setModel(IQCADItemModel* model);

    void setBackgroundParsingEnabled(bool enabled);
    bool backgroundParsingEnabled() const;

    /**
     * @brief lastParseResult
     * the result of the most recent parse. Might not match the current script.
     */
    ISCADParseResultPtr lastParseResult() const;

    /**
     * @brief discardParseResult
     * forget the last parse result, e.g. after the cache has been cleared.
     * The next rebuild parses again.
     */
    void discardParseResult();

    /**
     * @brief rebuild
     * rebuild the current script.
     * Waits for a pending/running background parse and reuses its result.
     * A running explicit rebuild is cancelled.
     */
    void rebuild(Task executeUntilTask);

    /**
     * @brief rebuildScript
     * parse and rebuild the given script (e.g. the script up to the cursor)
     */
    void rebuildScript(const std::string& script, Task executeUntilTask);

    /**
     * @brief stop
     * cancel everything, including a scheduled rebuild
     */
    void stop();

Q_SIGNALS:
    /**
     * @brief jobStarted
     * a new job was started
     */
    void jobStarted(IQISCADScriptModelGenerator::Task task, bool reusesParseResult);

    /**
     * @brief waitingForBackgroundParse
     * a rebuild was requested and will start after the running background parse has finished
     */
    void waitingForBackgroundParse();

    void scriptError(long failpos, QString errorMsg, int range, IQISCADScriptModelGenerator::Task task);
    void statusMessage(const QString& msg, double timeout=0);
    void statusProgress(int step, int totalSteps);
    void parsed(ISCADParseResultPtr parseResult, IQISCADScriptModelGenerator::Task task);
    void modelRebuilt();

    /**
     * @brief jobEnded
     * the current job has ended (in any case: success, error or cancellation)
     */
    void jobEnded(IQISCADScriptModelGenerator::Task task);
};




#endif // IQISCADSCRIPTJOBSCHEDULER_H
