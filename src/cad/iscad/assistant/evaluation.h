#ifndef INSIGHT_ASSISTANT_EVALUATION_H
#define INSIGHT_ASSISTANT_EVALUATION_H

#include <functional>
#include <memory>

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

#include "llmbackend.h"
#include "iscadassistanthost.h"

namespace insight {
namespace assistant {


/**
 * a modeling task for the automated evaluation of the assistant.
 *
 * JSON format:
 * {
 *   "id": "box_with_hole",
 *   "category": "create",
 *   "description": "...",
 *   "prompt": "the request to the assistant",
 *   "script": "initial script" | "script_file": "file.iscad" (relative to the task file),
 *   "max_steps": 30,
 *   "start_builds": true,
 *   "solution": "reference solution script", "solution_answer": "reference answer",
 *   "checks": [ { "type": "...", ... }, ... ]
 * }
 *
 * Check types:
 *  - builds: the final script builds without error
 *  - script_contains / script_not_contains: {"text"}
 *  - script_matches / script_not_matches: {"regex"}
 *  - script_unchanged: the script was not modified
 *  - scalar: {"name", "value"} value of a scalar symbol of the model
 *  - feature: {"feature": name or "*" (any feature), "component": bool (with "*": components only),
 *              "volume", "area", "bbox_size": [x,y,z], "bbox_min", "bbox_max", "cog": [x,y,z],
 *              "n_solids", "n_faces"}
 *  - feature_count: {"min", "max", "component": bool}
 *  - answer_number: {"value"} the final answer contains this number
 *  - answer_contains: {"text"} (case insensitive)
 * Numeric comparisons use "rel_tol" (default 0.005) and "abs_tol" (default 0.02).
 * Bounding boxes are computed from a tessellation: their tolerance is at least
 * "bbox_rel_tol" (default 0.01) times the largest dimension.
 * Every check may have a "description".
 */
struct EvalTask
{
    QString id, category, description, prompt, script, fileName;
    QJsonArray checks;
    int maxSteps = 0;

    /**
     * false for tasks, whose initial script is broken on purpose ("start_builds")
     */
    bool startBuilds = true;

    /**
     * reference solution (script and final answer) for validating the task:
     * the checks must pass for it ("solution", "solution_answer")
     */
    QString solution, solutionAnswer;

    static EvalTask fromJson(const QJsonObject& o, const QString& baseDir, QString* error);

    /**
     * all *.json files of a directory, sorted by file name
     */
    static QList<EvalTask> loadDirectory(const QString& dir, QString* error);
};


struct CheckResult
{
    QString description;
    bool passed = false;
    QString message;
};


struct EvalContext
{
    ISCADAssistantHost* host = nullptr;
    QString initialScript;
    bool finalBuildOk = false;
    QString finalBuildMessage;
    QString finalAnswer;
};


QList<CheckResult> evaluateChecks(const QJsonArray& checks, const EvalContext& ctx);


struct TaskResult
{
    QString taskId, category;
    int run = 0;
    bool passed = false;
    QList<CheckResult> checks;

    /**
     * reason, why the request ended (end_turn, max_steps, timeout, error, ...)
     */
    QString finishReason;
    QStringList errors;

    int modelResponses = 0, toolCalls = 0, failedToolCalls = 0;
    Usage usage;

    /**
     * USD, negative if unknown
     */
    double cost = -1;
    double seconds = 0;

    QString finalScript, finalAnswer;
    QJsonArray transcript;

    QJsonObject toJson() const;
};


/**
 * runs evaluation tasks with a model and checks the results
 */
class EvalRunner
    : public QObject
{
    Q_OBJECT

public:
    /**
     * creates the host for a task, initialized with the task's script.
     * The runner takes ownership.
     */
    typedef std::function<ISCADAssistantHost*(const EvalTask&)> HostFactory;

    /**
     * creates a new backend (the agent takes ownership)
     */
    typedef std::function<LLMBackend*()> BackendFactory;

    struct Config
    {
        QString systemPrompt;
        QStringList disabledTools;
        int maxSteps = 40;
        int timeoutSeconds = 900;
        int repeats = 1;
    };

private:
    HostFactory hostFactory_;
    BackendFactory backendFactory_;
    Config cfg_;

public:
    EvalRunner(HostFactory hf, BackendFactory bf, const Config& cfg, QObject* parent=nullptr);

    /**
     * run a single task (blocks, while processing events)
     */
    TaskResult runTask(const EvalTask& task, int run=0);

    QList<TaskResult> run(const QList<EvalTask>& tasks);

    /**
     * check the task definition without a model:
     * the initial script builds (or fails, if start_builds is false)
     * and the checks pass for the reference solution
     * @return the problems found (empty: ok)
     */
    QStringList validateTask(const EvalTask& task);

    static QJsonObject report(const QList<TaskResult>& results, const QJsonObject& meta);
    static QString markdownReport(const QList<TaskResult>& results, const QJsonObject& meta);

Q_SIGNALS:
    void taskStarted(const QString& id, int run);
    void taskFinished(const insight::assistant::TaskResult& result);
    void progress(const QString& message);
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_EVALUATION_H
