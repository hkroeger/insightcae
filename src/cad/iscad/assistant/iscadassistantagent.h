#ifndef INSIGHT_ASSISTANT_ISCADASSISTANTAGENT_H
#define INSIGHT_ASSISTANT_ISCADASSISTANTAGENT_H

#include <functional>

#include <QObject>
#include <QPointer>

#include "llmbackend.h"
#include "iscadassistanthost.h"

namespace insight {
namespace assistant {


/**
 * runs the conversation with the model:
 * sends the user request, executes the requested tool calls on the
 * model script and returns the results, until the model has finished.
 */
class ISCADAssistantAgent
    : public QObject
{
    Q_OBJECT

    ISCADAssistantHost* host_;
    QPointer<LLMBackend> backend_;

    QString systemPrompt_;
    QList<ToolDefinition> tools_;
    QStringList disabledTools_;
    QList<ChatMessage> history_;

    int maxSteps_ = 50;
    int step_ = 0;
    bool running_ = false;

    /**
     * incremented with each request and stop:
     * invalidates callbacks of outdated tool executions
     */
    quint64 runId_ = 0;

    /**
     * tool calls of the current model response
     */
    QList<ToolCall> pendingCalls_;
    QList<ToolResult> pendingResults_;
    bool rebuildRunning_ = false;

    /**
     * script contents at the end of the last request
     * (to detect user modifications in between)
     */
    QString lastKnownScript_;

    Usage sessionUsage_;

    void requestNextResponse();
    void onTurnFinished(const insight::assistant::TurnResult& result);
    void onTurnFailed(const QString& msg);
    void executeNextToolCall();
    void finishRequest(const QString& reason);

    /**
     * add results for unanswered tool calls at the end of the history
     */
    void resolveOpenToolCalls(const QString& reason);

public:
    ISCADAssistantAgent(ISCADAssistantHost* host, QObject* parent=nullptr);
    ~ISCADAssistantAgent();

    /**
     * the agent takes ownership
     */
    void setBackend(LLMBackend* backend);
    inline LLMBackend* backend() const { return backend_; }

    void setSystemPrompt(const QString& systemPrompt);
    inline const QString& systemPrompt() const { return systemPrompt_; }

    inline void setMaxSteps(int n) { maxSteps_=n; }

    /**
     * tools, which are not offered to the model
     * (e.g. view_model for models without image input)
     */
    void setDisabledTools(const QStringList& names);
    inline const QList<ToolDefinition>& tools() const { return tools_; }

    /**
     * the default system prompt
     * @param languageReference
     * documentation of the ISCAD language (markdown)
     */
    static QString defaultSystemPrompt(const QString& languageReference);

    static QList<ToolDefinition> toolDefinitions();

    /**
     * a reduced tool set for small (local) models
     */
    static QStringList basicToolSet();

    /**
     * names of all tools, which are not in the given set
     */
    static QStringList toolsNotIn(const QStringList& toolSet);

    static QString formatSymbols(const ISCADAssistantHost::SymbolTable& t);
    static QString formatFeatureProperties(
        const QString& name, const ISCADAssistantHost::FeatureProperties& p );

    inline bool isRunning() const { return running_; }
    inline const QList<ChatMessage>& history() const { return history_; }
    inline const Usage& sessionUsage() const { return sessionUsage_; }

    /**
     * execute a single tool call.
     * The callback is called exactly once (possibly synchronously).
     */
    void executeTool(const ToolCall& call, std::function<void(const ToolResult&)> done);

public Q_SLOTS:
    void sendUserMessage(const QString& text);
    void stop();
    void clearConversation();

Q_SIGNALS:
    void requestStarted();

    /**
     * the request has been processed
     * @param reason
     * "end_turn", "max_steps", "stopped", "error", "refusal", ...
     */
    void requestFinished(const QString& reason);

    void assistantTextDelta(const QString& text);
    void assistantThinkingDelta(const QString& text);

    /**
     * a model response has been completed
     */
    void assistantMessageCompleted(const QString& text);

    void toolCallStarted(const QString& name, const QString& description);
    void toolCallFinished(const QString& name, const QString& resultSummary, bool isError);

    /**
     * a tool returned an image (e.g. a view of the model)
     */
    void toolImageProduced(const QString& mediaType, const QByteArray& data);

    void errorOccurred(const QString& message);
    void statusMessage(const QString& message);
    void usageUpdated(const insight::assistant::Usage& sessionUsage);
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_ISCADASSISTANTAGENT_H
