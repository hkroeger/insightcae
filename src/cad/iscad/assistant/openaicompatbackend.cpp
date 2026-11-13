#include "openaicompatbackend.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <vector>

namespace insight {
namespace assistant {




// ================================================================
// tool calls in the text
// ================================================================


namespace {


QJsonValue typedParameterValue(QString value, const QJsonObject& propertySchema)
{
    // the format puts the value on lines of its own
    if (value.startsWith('\n'))
        value.remove(0, 1);
    if (value.endsWith('\n'))
        value.chop(1);

    QString type=propertySchema.value("type").toString();
    QString t=value.trimmed();
    if (type=="integer")
    {
        bool ok=false;
        qlonglong i=t.toLongLong(&ok);
        if (ok)
            return QJsonValue(i);
    }
    else if (type=="number")
    {
        bool ok=false;
        double d=t.toDouble(&ok);
        if (ok)
            return QJsonValue(d);
    }
    else if (type=="boolean")
    {
        if (t.compare("true", Qt::CaseInsensitive)==0)
            return QJsonValue(true);
        if (t.compare("false", Qt::CaseInsensitive)==0)
            return QJsonValue(false);
    }
    else if (type=="array" || type=="object")
    {
        QJsonParseError err;
        auto doc=QJsonDocument::fromJson(t.toUtf8(), &err);
        if (err.error==QJsonParseError::NoError)
        {
            if (type=="array" && doc.isArray())
                return doc.array();
            if (type=="object" && doc.isObject())
                return doc.object();
        }
    }
    return QJsonValue(value);
}


} // anonymous namespace


QList<TextToolCall> extractTextToolCalls(QString& text, const QList<ToolDefinition>& tools)
{
    QList<TextToolCall> calls;
    if (!text.contains("<function="))
        return calls;

    static const QRegularExpression
        reFunction(
            "<function=([^>\\s]+)>(.*?)(?:</function>|(?=<function=)|$)",
            QRegularExpression::DotMatchesEverythingOption ),
        reParameter(
            "<parameter=([^>\\s]+)>(.*?)(?:</parameter>|(?=<parameter=)|$)",
            QRegularExpression::DotMatchesEverythingOption ),
        reToolCallTag("</?tool_call>");

    QString remaining;
    int last=0;
    auto it=reFunction.globalMatch(text);
    while (it.hasNext())
    {
        auto m=it.next();
        remaining+=text.mid(last, m.capturedStart()-last);
        last=m.capturedEnd();

        TextToolCall tc;
        tc.name=m.captured(1);
        QJsonObject properties;
        for (const auto& t: tools)
            if (t.name==tc.name)
                properties=t.inputSchema.value("properties").toObject();

        auto pit=reParameter.globalMatch(m.captured(2));
        while (pit.hasNext())
        {
            auto pm=pit.next();
            QString pn=pm.captured(1);
            tc.input[pn]=typedParameterValue(pm.captured(2), properties.value(pn).toObject());
        }
        calls.append(tc);
    }
    remaining+=text.mid(last);

    if (!calls.isEmpty())
    {
        remaining.remove(reToolCallTag);
        text=remaining.trimmed();
    }
    return calls;
}


QString textToolInstructions(const QList<ToolDefinition>& tools)
{
    QString s=
        "# Tools\n\n"
        "You can call the following tools. Write each tool call in exactly this format "
        "(all tags are required, one parameter block per parameter):\n\n"
        "<tool_call>\n<function=TOOL_NAME>\n<parameter=PARAMETER_NAME>\nvalue\n</parameter>\n"
        "</function>\n</tool_call>\n\n"
        "Available tools:\n";
    for (const auto& t: tools)
    {
        s+=QString("\n- %1: %2\n  parameters (JSON schema): %3\n")
             .arg(t.name, t.description,
                  QString::fromUtf8(QJsonDocument(t.inputSchema).toJson(QJsonDocument::Compact)));
    }
    return s;
}




// ================================================================
// OpenAICompatStreamAccumulator
// ================================================================


OpenAICompatStreamAccumulator::OpenAICompatStreamAccumulator(const QList<ToolDefinition>& tools)
    : tools_(tools)
{}



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

    // fallback: tool calls written into the text, which the server did not recognize
    // (also mixed with recognized calls: the server may miss only the first one of several).
    // The text precedes the recognized calls in the output.
    std::vector<PartialToolCall> toolCalls;
    int i=0;
    for (const auto& ttc: extractTextToolCalls(m.text, tools_))
    {
        PartialToolCall p;
        p.id=QString("call_text_%1").arg(i++);
        p.name=ttc.name;
        p.arguments=QString::fromUtf8(QJsonDocument(ttc.input).toJson(QJsonDocument::Compact));
        toolCalls.push_back(p);
    }
    for (const auto& e: toolCalls_)
    {
        toolCalls.push_back(e.second);
        if (toolCalls.back().id.isEmpty())
            toolCalls.back().id=QString("call_%1").arg(e.first);
    }

    QJsonArray tcs;
    for (const auto& e: toolCalls)
    {
        ToolCall tc;
        tc.id=e.id;
        tc.name=e.name;
        tc.rawInput=e.arguments;

        QString args=e.arguments.trimmed();
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
    pc["content"] = m.text.isEmpty() && !tcs.isEmpty() ? QJsonValue() : QJsonValue(m.text);
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
    return std::make_unique<OpenAICompatStreamAccumulator>(tools_);
}


bool OpenAICompatBackend::alternativeRequest(int httpStatus, const QByteArray& errorBody, QByteArray& requestBody)
{
    if (httpStatus!=500 || toolsInText_ || tools_.isEmpty())
        return false;

    QString b=QString::fromUtf8(errorBody);
    if (!( b.contains("XML syntax error", Qt::CaseInsensitive)
           || b.contains("parsing tool call", Qt::CaseInsensitive)
           || b.contains("parse tool call", Qt::CaseInsensitive) ))
        return false;

    toolsInText_=true;
    requestBody=QJsonDocument(buildRequest(cfg_, id(), systemPrompt_, history_, tools_, true))
                    .toJson(QJsonDocument::Compact);
    return true;
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
    const QList<ToolDefinition>& tools,
    bool toolsInText )
{
    QJsonObject req;
    req["model"]=cfg.model;
    req["stream"]=true;
    if (cfg.streamUsage)
        req["stream_options"]=QJsonObject{ {"include_usage", true} };
    if (cfg.maxTokens>0)
        req["max_tokens"]=cfg.maxTokens;

    if (!tools.isEmpty() && !toolsInText)
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
    QString sp=systemPrompt;
    if (!tools.isEmpty() && toolsInText)
        sp+=(sp.isEmpty() ? "" : "\n\n")+textToolInstructions(tools);
    if (!sp.isEmpty())
        messages.append(QJsonObject{ {"role", "system"}, {"content", sp} });

    for (const auto& m: history)
    {
        if (m.role==ChatMessage::User)
        {
            // tool messages carry text only: images follow in a user message
            QJsonArray imageParts;
            for (const auto& tr: m.toolResults)
            {
                QString content = tr.isError ? "ERROR: "+tr.content : tr.content;
                if (!tr.images.isEmpty())
                    content+=QString("\n[%1 image(s) attached in the following message]").arg(tr.images.size());
                messages.append(QJsonObject{
                    {"role", "tool"},
                    {"tool_call_id", tr.toolCallId},
                    {"content", content}
                });
                for (const auto& img: tr.images)
                {
                    if (imageParts.isEmpty())
                        imageParts.append(QJsonObject{
                            {"type", "text"},
                            {"text", "Images returned by the tool calls:"} });
                    imageParts.append(QJsonObject{
                        {"type", "image_url"},
                        {"image_url", QJsonObject{
                             {"url", QString("data:%1;base64,%2")
                                     .arg(img.mediaType, QString::fromLatin1(img.data.toBase64()))}
                         }}
                    });
                }
            }
            if (!imageParts.isEmpty())
            {
                if (!m.text.isEmpty())
                    imageParts.append(QJsonObject{ {"type", "text"}, {"text", m.text} });
                messages.append(QJsonObject{ {"role", "user"}, {"content", imageParts} });
            }
            else if (!m.text.isEmpty())
            {
                messages.append(QJsonObject{ {"role", "user"}, {"content", m.text} });
            }
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

    systemPrompt_=systemPrompt;
    history_=history;
    tools_=tools;
    toolsInText_=false;

    startTurn(
        req,
        QJsonDocument(buildRequest(cfg_, id(), systemPrompt, history, tools))
            .toJson(QJsonDocument::Compact) );
}


} // namespace assistant
} // namespace insight
