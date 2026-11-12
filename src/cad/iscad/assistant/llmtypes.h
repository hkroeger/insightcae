#ifndef INSIGHT_ASSISTANT_LLMTYPES_H
#define INSIGHT_ASSISTANT_LLMTYPES_H

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonValue>

namespace insight {
namespace assistant {


/**
 * a tool call requested by the model
 */
struct ToolCall
{
    QString id;
    QString name;
    QJsonObject input;

    /**
     * the input as received (for error reporting, if it could not be parsed)
     */
    QString rawInput;

    /**
     * false, if the input was no valid JSON object
     */
    bool inputValid = true;
};


struct ToolResult
{
    QString toolCallId;
    QString content;
    bool isError = false;
};


/**
 * provider independent representation of a conversation message
 */
struct ChatMessage
{
    enum Role { User, Assistant };
    Role role = User;

    /**
     * user: the prompt (may be empty, if only tool results are returned).
     * assistant: the concatenated text output
     */
    QString text;

    /**
     * assistant only
     */
    QList<ToolCall> toolCalls;

    /**
     * user only: results of the tool calls of the preceding assistant message
     */
    QList<ToolResult> toolResults;

    /**
     * assistant only: the message content exactly as returned by the provider.
     * Is sent back unchanged by the same backend
     * (required e.g. for thinking blocks, which carry signatures).
     */
    QJsonValue providerContent;

    /**
     * the backend, which created providerContent
     */
    QString providerId;
};


struct ToolDefinition
{
    QString name;
    QString description;
    QJsonObject inputSchema;
};


struct Usage
{
    qint64 inputTokens = 0;
    qint64 outputTokens = 0;
    qint64 cacheReadTokens = 0;
    qint64 cacheWriteTokens = 0;

    Usage& operator+=(const Usage& o)
    {
        inputTokens+=o.inputTokens;
        outputTokens+=o.outputTokens;
        cacheReadTokens+=o.cacheReadTokens;
        cacheWriteTokens+=o.cacheWriteTokens;
        return *this;
    }

    Usage operator-(const Usage& o) const
    {
        Usage r=*this;
        r.inputTokens-=o.inputTokens;
        r.outputTokens-=o.outputTokens;
        r.cacheReadTokens-=o.cacheReadTokens;
        r.cacheWriteTokens-=o.cacheWriteTokens;
        return r;
    }

    inline qint64 total() const
    {
        return inputTokens+outputTokens+cacheReadTokens+cacheWriteTokens;
    }
};


/**
 * prices in USD per million tokens
 */
struct ModelPricing
{
    bool known = false;
    double input = 0, output = 0, cacheRead = 0, cacheWrite = 0;

    inline double cost(const Usage& u) const
    {
        return 1e-6*(
            input*double(u.inputTokens)
            + output*double(u.outputTokens)
            + cacheRead*double(u.cacheReadTokens)
            + cacheWrite*double(u.cacheWriteTokens) );
    }
};


struct TurnResult
{
    ChatMessage message;

    /**
     * normalized stop reason:
     * "end_turn", "tool_use", "max_tokens", "refusal" or a provider specific value
     */
    QString stopReason;

    /**
     * the model which actually answered (may differ from the requested one after a fallback)
     */
    QString model;

    Usage usage;
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_LLMTYPES_H
