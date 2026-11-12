#include "openaicompatbackend.h"

#include <QJsonDocument>
#include <QJsonArray>

namespace insight {
namespace assistant {




// ================================================================
// OpenAICompatStreamAccumulator
// ================================================================


StreamAccumulator::Status
OpenAICompatStreamAccumulator::processEvent(const SSEStreamParser::Event& ev)
{
    if (status_!=InProgress)
        return status_;

    QByteArray data=ev.data.trimmed();
    if (data.isEmpty())
        return status_;

    if (data=="[DONE]")
    {
        status_=Done;
        return status_;
    }

    QJsonParseError err;
    auto doc=QJsonDocument::fromJson(data, &err);
    if (err.error!=QJsonParseError::NoError || !doc.isObject())
    {
        setError(QString(), QString("invalid data in response stream: %1").arg(err.errorString()));
        return status_;
    }
    auto d=doc.object();

    if (d.contains("error"))
    {
        auto e=d.value("error");
        if (e.isObject())
            setError(e.toObject().value("type").toString(), e.toObject().value("message").toString());
        else
            setError(QString(), e.toVariant().toString());
        return status_;
    }

    if (d.contains("model"))
        model_=d.value("model").toString();

    auto u=d.value("usage").toObject();
    if (!u.isEmpty())
    {
        qint64 prompt=u.value("prompt_tokens").toVariant().toLongLong();
        qint64 cached=u.value("prompt_tokens_details").toObject()
                        .value("cached_tokens").toVariant().toLongLong();
        usage_.inputTokens=prompt-cached;
        usage_.cacheReadTokens=cached;
        usage_.outputTokens=u.value("completion_tokens").toVariant().toLongLong();
    }

    for (const auto& cv: d.value("choices").toArray())
    {
        auto c=cv.toObject();
        if (c.value("index").toInt(0)!=0)
            continue;

        auto delta=c.value("delta").toObject();

        QString t=delta.value("content").toString();
        if (!t.isEmpty())
        {
            receivedContent_=true;
            text_+=t;
            if (onText) onText(t);
        }

        // reasoning output of some servers (not sent back)
        for (const char* key: {"reasoning_content", "reasoning"})
        {
            QString r=delta.value(key).toString();
            if (!r.isEmpty())
            {
                receivedContent_=true;
                if (onThinking) onThinking(r);
            }
        }

        for (const auto& tcv: delta.value("tool_calls").toArray())
        {
            auto tc=tcv.toObject();
            receivedContent_=true;

            int idx;
            if (tc.contains("index"))
            {
                idx=tc.value("index").toInt();
            }
            else
            {
                // index missing: a new id starts a new call
                QString tid=tc.value("id").toString();
                if (lastToolIndex_<0
                    || (!tid.isEmpty() && toolCalls_[lastToolIndex_].id!=tid
                        && !toolCalls_[lastToolIndex_].id.isEmpty()))
                    idx=int(toolCalls_.size());
                else
                    idx=lastToolIndex_;
            }
            lastToolIndex_=idx;

            auto& p=toolCalls_[idx];
            if (tc.contains("id") && !tc.value("id").toString().isEmpty())
                p.id=tc.value("id").toString();
            auto f=tc.value("function").toObject();
            if (f.contains("name"))
                p.name+=f.value("name").toString();
            if (f.value("arguments").isString())
                p.arguments+=f.value("arguments").toString();
            else if (f.value("arguments").isObject())
                // some servers send complete objects
                p.arguments=QString::fromUtf8(
                    QJsonDocument(f.value("arguments").toObject()).toJson(QJsonDocument::Compact));
        }

        auto fr=c.value("finish_reason");
        if (fr.isString() && !fr.toString().isEmpty())
        {
            finishReason_=fr.toString();
            finished_=true;
        }
    }

    return status_;
}


StreamAccumulator::Status OpenAICompatStreamAccumulator::finishStream()
{
    // some servers close the stream without [DONE]
    if (status_==InProgress && finished_)
        status_=Done;
    return status_;
}


TurnResult OpenAICompatStreamAccumulator::result(const QString& providerId) const
{
    TurnResult r;
    r.model=model_;
    r.usage=usage_;

    auto& m=r.message;
    m.role=ChatMessage::Assistant;
    m.providerId=providerId;
    m.text=text_;

    QJsonArray tcs;
    for (const auto& e: toolCalls_)
    {
        ToolCall tc;
        tc.id = e.second.id.isEmpty() ? QString("call_%1").arg(e.first) : e.second.id;
        tc.name=e.second.name;
        tc.rawInput=e.second.arguments;

        QString args=e.second.arguments.trimmed();
        if (args.isEmpty())
        {
            tc.input=QJsonObject();
            args="{}";
        }
        else
        {
            QJsonParseError err;
            auto doc=QJsonDocument::fromJson(args.toUtf8(), &err);
            if (err.error==QJsonParseError::NoError && doc.isObject())
                tc.input=doc.object();
            else
                tc.inputValid=false;
        }
        m.toolCalls.append(tc);

        tcs.append(QJsonObject{
            {"id", tc.id},
            {"type", "function"},
            {"function", QJsonObject{ {"name", tc.name}, {"arguments", args} }}
        });
    }

    QJsonObject pc{ {"role", "assistant"} };
    pc["content"] = text_.isEmpty() && !tcs.isEmpty() ? QJsonValue() : QJsonValue(text_);
    if (!tcs.isEmpty())
        pc["tool_calls"]=tcs;
    m.providerContent=pc;

    if (finishReason_=="length")
        r.stopReason="max_tokens";
    else if (finishReason_=="content_filter")
        r.stopReason="refusal";
    else if (!m.toolCalls.isEmpty())
        r.stopReason="tool_use"; // some servers report "stop" with tool calls
    else if (finishReason_=="stop" || finishReason_.isEmpty())
        r.stopReason="end_turn";
    else
        r.stopReason=finishReason_;

    return r;
}




// ================================================================
// OpenAICompatBackend
// ================================================================


OpenAICompatBackend::OpenAICompatBackend(const OpenAICompatConfig& cfg, QObject* parent)
    : StreamingHttpBackend(parent),
      cfg_(cfg)
{
    idleTimeoutSeconds_=cfg_.idleTimeoutSeconds;
    maxRetries_=cfg_.maxRetries;
}


QString OpenAICompatBackend::id() const
{
    return "openai-compat";
}


QString OpenAICompatBackend::description() const
{
    return QString("%1 at %2").arg(cfg_.model, cfg_.baseUrl.toString());
}


ModelPricing OpenAICompatBackend::pricing() const
{
    ModelPricing p;
    p.input=cfg_.priceInput;
    p.output=cfg_.priceOutput;
    p.cacheRead=cfg_.priceCachedInput;
    p.cacheWrite=cfg_.priceInput;
    p.known = cfg_.priceInput>0 || cfg_.priceOutput>0;
    return p;
}


std::unique_ptr<StreamAccumulator> OpenAICompatBackend::createAccumulator() const
{
    return std::make_unique<OpenAICompatStreamAccumulator>();
}


bool OpenAICompatBackend::isRetryableStreamError(const QString& errorType) const
{
    return errorType=="server_error" || errorType=="rate_limit_exceeded"
           || StreamingHttpBackend::isRetryableStreamError(errorType);
}


QJsonObject OpenAICompatBackend::buildRequest(
    const OpenAICompatConfig& cfg,
    const QString& providerId,
    const QString& systemPrompt,
    const QList<ChatMessage>& history,
    const QList<ToolDefinition>& tools )
{
    QJsonObject req;
    req["model"]=cfg.model;
    req["stream"]=true;
    if (cfg.streamUsage)
        req["stream_options"]=QJsonObject{ {"include_usage", true} };
    if (cfg.maxTokens>0)
        req["max_tokens"]=cfg.maxTokens;

    if (!tools.isEmpty())
    {
        QJsonArray ta;
        for (const auto& t: tools)
        {
            ta.append(QJsonObject{
                {"type", "function"},
                {"function", QJsonObject{
                     {"name", t.name},
                     {"description", t.description},
                     {"parameters", t.inputSchema}
                 }}
            });
        }
        req["tools"]=ta;
    }

    QJsonArray messages;
    if (!systemPrompt.isEmpty())
        messages.append(QJsonObject{ {"role", "system"}, {"content", systemPrompt} });

    for (const auto& m: history)
    {
        if (m.role==ChatMessage::User)
        {
            for (const auto& tr: m.toolResults)
            {
                messages.append(QJsonObject{
                    {"role", "tool"},
                    {"tool_call_id", tr.toolCallId},
                    {"content", tr.isError ? "ERROR: "+tr.content : tr.content}
                });
            }
            if (!m.text.isEmpty())
                messages.append(QJsonObject{ {"role", "user"}, {"content", m.text} });
        }
        else
        {
            if (m.providerId==providerId && m.providerContent.isObject())
            {
                messages.append(m.providerContent.toObject());
            }
            else
            {
                QJsonObject am{ {"role", "assistant"} };
                QJsonArray tcs;
                for (const auto& tc: m.toolCalls)
                {
                    tcs.append(QJsonObject{
                        {"id", tc.id},
                        {"type", "function"},
                        {"function", QJsonObject{
                             {"name", tc.name},
                             {"arguments", QString::fromUtf8(
                                  QJsonDocument(tc.input).toJson(QJsonDocument::Compact))}
                         }}
                    });
                }
                am["content"] = m.text.isEmpty() && !tcs.isEmpty() ? QJsonValue() : QJsonValue(m.text);
                if (!tcs.isEmpty())
                    am["tool_calls"]=tcs;
                messages.append(am);
            }
        }
    }
    req["messages"]=messages;

    return req;
}


QStringList OpenAICompatBackend::parseModelList(const QByteArray& response)
{
    QStringList ids;
    auto doc=QJsonDocument::fromJson(response);
    for (const auto& v: doc.object().value("data").toArray())
    {
        QString id=v.toObject().value("id").toString();
        if (!id.isEmpty())
            ids<<id;
    }
    ids.sort(Qt::CaseInsensitive);
    return ids;
}


void OpenAICompatBackend::sendTurn(
    const QString& systemPrompt,
    const QList<ChatMessage>& history,
    const QList<ToolDefinition>& tools )
{
    if (cfg_.model.isEmpty())
    {
        failLater("No model configured. Select a model in the assistant settings.");
        return;
    }

    QNetworkRequest req(endpoint(cfg_.baseUrl, "/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("accept", "text/event-stream");
    if (!cfg_.apiKey.isEmpty())
        req.setRawHeader("Authorization", "Bearer "+cfg_.apiKey.toUtf8());

    startTurn(
        req,
        QJsonDocument(buildRequest(cfg_, id(), systemPrompt, history, tools))
            .toJson(QJsonDocument::Compact) );
}


} // namespace assistant
} // namespace insight
