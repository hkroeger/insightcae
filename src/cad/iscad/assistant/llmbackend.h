#ifndef INSIGHT_ASSISTANT_LLMBACKEND_H
#define INSIGHT_ASSISTANT_LLMBACKEND_H

#include <QObject>

#include "llmtypes.h"

namespace insight {
namespace assistant {


/**
 * interface to a chat model with tool use.
 * One turn (one model response) is processed at a time.
 * Lives in the GUI thread, all signals are emitted there.
 */
class LLMBackend
    : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    /**
     * identifies the provider format of ChatMessage::providerContent
     */
    virtual QString id() const =0;

    /**
     * human readable description, e.g. "Anthropic claude-opus-5-5"
     */
    virtual QString description() const =0;

    /**
     * request the next model response.
     * Exactly one of turnFinished or turnFailed is emitted afterwards,
     * unless cancel() is called before.
     */
    virtual void sendTurn(
        const QString& systemPrompt,
        const QList<ChatMessage>& history,
        const QList<ToolDefinition>& tools ) =0;

    virtual void cancel() =0;

    /**
     * token prices of the used model (known=false, if unknown)
     */
    virtual ModelPricing pricing() const { return ModelPricing(); }

    /**
     * size of the context window of the model in tokens (0: unknown).
     * Used to warn, before the server silently cuts off input.
     */
    virtual int contextLength() const { return 0; }

    virtual bool isBusy() const =0;

Q_SIGNALS:
    void textDelta(const QString& text);
    void thinkingDelta(const QString& text);

    /**
     * a status information, e.g. about a retry
     */
    void statusMessage(const QString& msg);

    void turnFinished(const insight::assistant::TurnResult& result);
    void turnFailed(const QString& errorMessage);
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_LLMBACKEND_H
