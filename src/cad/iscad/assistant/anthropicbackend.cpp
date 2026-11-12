#include "anthropicbackend.h"

#include <QJsonDocument>
#include <QJsonArray>

#include <map>

namespace insight {
namespace assistant {


namespace {

const char* apiVersion = "2026-10-21";
const char* fallbackBeta = "server-side-fallback-2026-07-01";

}




// ================================================================
// AnthropicStreamAccumulator
// ================================================================


void AnthropicStreamAccumulator::finishBlock(size_t i)
{
    auto& b = blocks_[i];
    if (b.value("type").toString()=="tool_use")
    {
        const QString& pj = partialJson_[i];
        if (pj.trimmed().isEmpty())
        {
            if (!b.value("input").isObject())
                b["input"]=QJsonObject();
        }
        else
        {
            QJsonParseError err;
            auto doc = QJsonDocument::fromJson(pj.toUtf8(), &err);
            if (err.error==QJsonParseError::NoError && doc.isObject())
            {
                b["input"]=doc.object();
            }
            else
            {
                // keep a valid block for the history, mark the call as invalid
                b["input"]=QJsonObject();
                b["__invalid_input"]=pj;
            }
        }
    }
}


AnthropicStreamAccumulator::Status
AnthropicStreamAccumulator::processEvent(const SSEStreamParser::Event& ev)
{
    if (status_!=InProgress)
        return status_;

    if (ev.data.isEmpty())
        return status_;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(ev.data, &err);
    if (err.error!=QJsonParseError::NoError || !doc.isObject())
    {
        setError(QString(), QString("invalid event data in response stream: %1").arg(err.errorString()));
        return status_;
    }
    auto d = doc.object();
    QString type = d.value("type").toString();

    auto readUsage = [this](const QJsonObject& u)
    {
        if (u.contains("input_tokens"))
            usage_.inputTokens=u.value("input_tokens").toVariant().toLongLong();
        if (u.contains("output_tokens"))
            usage_.outputTokens=u.value("output_tokens").toVariant().toLongLong();
        if (u.contains("cache_read_input_tokens"))
            usage_.cacheReadTokens=u.value("cache_read_input_tokens").toVariant().toLongLong();
        if (u.contains("cache_creation_input_tokens"))
            usage_.cacheWriteTokens=u.value("cache_creation_input_tokens").toVariant().toLongLong();
    };

    if (type=="message_start")
    {
        auto m = d.value("message").toObject();
        model_ = m.value("model").toString();
        readUsage(m.value("usage").toObject());
    }
    else if (type=="content_block_start")
    {
        size_t i = size_t(d.value("index").toInt());
        if (blocks_.size()<=i)
        {
            blocks_.resize(i+1);
            partialJson_.resize(i+1);
        }
        blocks_[i]=d.value("content_block").toObject();
        receivedContent_=true;
    }
    else if (type=="content_block_delta")
    {
        size_t i = size_t(d.value("index").toInt());
        if (i>=blocks_.size())
        {
            setError(QString(), "delta for unknown content block in response stream");
            return status_;
        }
        auto& b = blocks_[i];
        auto delta = d.value("delta").toObject();
        QString dt = delta.value("type").toString();
        if (dt=="text_delta")
        {
            QString t=delta.value("text").toString();
            b["text"]=b.value("text").toString()+t;
            if (onText) onText(t);
        }
        else if (dt=="thinking_delta")
        {
            QString t=delta.value("thinking").toString();
            b["thinking"]=b.value("thinking").toString()+t;
            if (onThinking) onThinking(t);
        }
        else if (dt=="signature_delta")
        {
            b["signature"]=b.value("signature").toString()+delta.value("signature").toString();
        }
        else if (dt=="input_json_delta")
        {
            partialJson_[i]+=delta.value("partial_json").toString();
        }
        else if (dt=="citations_delta")
        {
            auto c=b.value("citations").toArray();
            c.append(delta.value("citation"));
            b["citations"]=c;
        }
        // other delta types are not needed for the history
    }
    else if (type=="content_block_stop")
    {
        size_t i = size_t(d.value("index").toInt());
        if (i<blocks_.size())
            finishBlock(i);
    }
    else if (type=="message_delta")
    {
        auto delta = d.value("delta").toObject();
        if (delta.contains("stop_reason") && !delta.value("stop_reason").isNull())
            stopReason_=delta.value("stop_reason").toString();
        readUsage(d.value("usage").toObject());
    }
    else if (type=="message_stop")
    {
        status_=Done;
    }
    else if (type=="error")
    {
        auto e=d.value("error").toObject();
        setError(e.value("type").toString(), e.value("message").toString());
    }
    // "ping" and unknown events are ignored

    return status_;
}


TurnResult AnthropicStreamAccumulator::result(const QString& providerId) const
{
    TurnResult r;
    r.stopReason=stopReason_;
    r.model=model_;
    r.usage=usage_;

    auto& m=r.message;
    m.role=ChatMessage::Assistant;
    m.providerId=providerId;

    QJsonArray content;
    for (auto b: blocks_)
    {
        if (b.isEmpty())
            continue;

        QString type=b.value("type").toString();
        if (type=="text")
        {
            if (!m.text.isEmpty() && !m.text.endsWith('\n'))
                m.text+="\n\n";
            m.text+=b.value("text").toString();
        }
        else if (type=="tool_use")
        {
            ToolCall tc;
            tc.id=b.value("id").toString();
            tc.name=b.value("name").toString();
            tc.input=b.value("input").toObject();
            if (b.contains("__invalid_input"))
            {
                tc.inputValid=false;
                tc.rawInput=b.value("__invalid_input").toString();
                b.remove("__invalid_input");
            }
            else
            {
                tc.rawInput=QString::fromUtf8(QJsonDocument(tc.input).toJson(QJsonDocument::Compact));
            }
            m.toolCalls.append(tc);
        }
        content.append(b);
    }
    m.providerContent=content;

    return r;
}




// ================================================================
// AnthropicBackend
// ================================================================


AnthropicBackend::AnthropicBackend(const AnthropicConfig& cfg, QObject* parent)
    : StreamingHttpBackend(parent),
      cfg_(cfg)
{
    idleTimeoutSeconds_=cfg_.idleTimeoutSeconds;
    maxRetries_=cfg_.maxRetries;
}


QString AnthropicBackend::id() const
{
    return "anthropic";
}


QString AnthropicBackend::description() const
{
    return QString("Anthropic %1").arg(cfg_.model);
}


ModelPricing AnthropicBackend::pricingOf(const QString& model)
{
    // USD per million tokens: input, output, cache read (cache write: 1.25 x input)
    struct P { double in, out, cacheRead; };
    static const std::map<QString, P> prices{
        {"claude-fable-5-1", {10., 50., 0.25}},
        {"claude-opus-5-5", {4., 20., 0.2}},
        {"claude-sonnet-5-5", {2., 10., 0.2}},
        {"claude-haiku-5-5", {0.1, 0.5, 0.01}}
    };
    ModelPricing r;
    auto i=prices.find(model);
    if (i!=prices.end())
    {
        r.known=true;
        r.input=i->second.in;
        r.output=i->second.out;
        r.cacheRead=i->second.cacheRead;
        r.cacheWrite=1.25*i->second.in;
    }
    return r;
}


ModelPricing AnthropicBackend::pricing() const
{
    return pricingOf(cfg_.model);
}


std::unique_ptr<StreamAccumulator> AnthropicBackend::createAccumulator() const
{
    return std::make_unique<AnthropicStreamAccumulator>();
}


bool AnthropicBackend::isFirstPartyEndpoint(const QUrl& baseUrl)
{
    return baseUrl.host()=="api.anthropic.com";
}


QJsonObject AnthropicBackend::buildRequest(
    const AnthropicConfig& cfg,
    const QString& providerId,
    const QString& systemPrompt,
    const QList<ChatMessage>& history,
    const QList<ToolDefinition>& tools )
{
    QJsonObject req;
    req["model"]=cfg.model;
    req["max_tokens"]=cfg.maxTokens;
    req["stream"]=true;

    // the system prompt is stable during a session:
    // cache it (together with the tool definitions, which precede it)
    if (!systemPrompt.isEmpty())
    {
        req["system"]=QJsonArray{ QJsonObject{
            {"type", "text"},
            {"text", systemPrompt},
            {"cache_control", QJsonObject{{"type", "ephemeral"}}}
        } };
    }

    if (!tools.isEmpty())
    {
        QJsonArray ta;
        for (const auto& t: tools)
        {
            ta.append(QJsonObject{
                {"name", t.name},
                {"description", t.description},
                {"input_schema", t.inputSchema},
                {"eager_input_streaming", true}
            });
        }
        req["tools"]=ta;
    }

    req["thinking"]=QJsonObject{ {"type", "adaptive"}, {"display", "summarized"} };
    if (!cfg.effort.isEmpty())
        req["output_config"]=QJsonObject{ {"effort", cfg.effort} };

    if (cfg.serverSideFallback && isFirstPartyEndpoint(cfg.baseUrl))
        req["fallbacks"]="default";

    QJsonArray messages;
    for (const auto& m: history)
    {
        QJsonArray content;
        if (m.role==ChatMessage::User)
        {
            for (const auto& tr: m.toolResults)
            {
                QJsonObject b{
                    {"type", "tool_result"},
                    {"tool_use_id", tr.toolCallId},
                    {"content", tr.content.isEmpty() ? QString("(empty)") : tr.content}
                };
                if (tr.isError)
                    b["is_error"]=true;
                content.append(b);
            }
            if (!m.text.isEmpty())
                content.append(QJsonObject{ {"type", "text"}, {"text", m.text} });
            messages.append(QJsonObject{ {"role", "user"}, {"content", content} });
        }
        else
        {
            if (m.providerId==providerId && m.providerContent.isArray())
            {
                content=m.providerContent.toArray();
            }
            else
            {
                if (!m.text.isEmpty())
                    content.append(QJsonObject{ {"type", "text"}, {"text", m.text} });
                for (const auto& tc: m.toolCalls)
                {
                    content.append(QJsonObject{
                        {"type", "tool_use"},
                        {"id", tc.id},
                        {"name", tc.name},
                        {"input", tc.input}
                    });
                }
            }
            if (content.isEmpty())
                content.append(QJsonObject{ {"type", "text"}, {"text", "(no output)"} });
            messages.append(QJsonObject{ {"role", "assistant"}, {"content", content} });
        }
    }

    // cache the conversation up to the latest message
    if (!messages.isEmpty())
    {
        auto last=messages.last().toObject();
        auto content=last.value("content").toArray();
        if (!content.isEmpty())
        {
            auto lb=content.last().toObject();
            lb["cache_control"]=QJsonObject{{"type", "ephemeral"}};
            content.replace(content.size()-1, lb);
            last["content"]=content;
            messages.replace(messages.size()-1, last);
        }
    }

    req["messages"]=messages;

    return req;
}


void AnthropicBackend::sendTurn(
    const QString& systemPrompt,
    const QList<ChatMessage>& history,
    const QList<ToolDefinition>& tools )
{
    if (cfg_.apiKey.isEmpty())
    {
        failLater("No Anthropic API key configured. "
                  "Set the environment variable ANTHROPIC_API_KEY or enter the key in the assistant settings.");
        return;
    }

    QNetworkRequest req(endpoint(cfg_.baseUrl, "/v1/messages"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-api-key", cfg_.apiKey.toUtf8());
    req.setRawHeader("anthropic-version", apiVersion);
    req.setRawHeader("accept", "text/event-stream");
    if (cfg_.serverSideFallback && isFirstPartyEndpoint(cfg_.baseUrl))
        req.setRawHeader("anthropic-beta", fallbackBeta);

    startTurn(
        req,
        QJsonDocument(buildRequest(cfg_, id(), systemPrompt, history, tools))
            .toJson(QJsonDocument::Compact) );
}


} // namespace assistant
} // namespace insight
