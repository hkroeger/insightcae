/*
 * tests of the ISCAD chat assistant:
 * SSE parsing, Anthropic stream assembly and request format,
 * the agent loop with a scripted model, and the HTTP client against a local server
 */

#include <QtTest/QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonArray>

#include "assistant/ssestreamparser.h"
#include "assistant/anthropicbackend.h"
#include "assistant/openaicompatbackend.h"
#include "assistant/iscadassistantagent.h"

using namespace insight::assistant;

Q_DECLARE_METATYPE(insight::assistant::TurnResult)


// ================================================================
// fakes
// ================================================================


/**
 * returns prepared responses and records the requests
 */
class FakeBackend : public LLMBackend
{
public:
    QList<TurnResult> responses;
    QList<QList<ChatMessage> > requests;
    bool failNext=false;
    bool holdResponse=false; // don't answer (to test stop)
    bool busy=false;

    using LLMBackend::LLMBackend;

    QString id() const override { return "fake"; }
    QString description() const override { return "fake"; }

    void sendTurn(const QString&, const QList<ChatMessage>& history, const QList<ToolDefinition>&) override
    {
        requests.append(history);
        busy=true;
        if (holdResponse)
            return;
        QTimer::singleShot(0, this, [this]()
        {
            if (!busy) return;
            busy=false;
            if (failNext || responses.isEmpty())
            {
                failNext=false;
                Q_EMIT turnFailed("fake failure");
                return;
            }
            Q_EMIT turnFinished(responses.takeFirst());
        });
    }
    void cancel() override { busy=false; }
    bool isBusy() const override { return busy; }
};


class FakeHost : public ISCADAssistantHost
{
public:
    QString script;
    int beginCount=0, endCount=0, editCount=0, rebuildCount=0, cancelCount=0;
    bool inTurn=false;
    bool holdRebuild=false;
    RebuildCallback pendingRebuild;

    QString scriptText() const override { return script; }
    QString scriptFileName() const override { return "test.iscad"; }
    void beginAssistantTurn() override { ++beginCount; inTurn=true; }
    void endAssistantTurn() override { ++endCount; inTurn=false; }
    void replaceText(int pos, int length, const QString& text) override
    {
        QVERIFY(inTurn);
        ++editCount;
        script.replace(pos, length, text);
    }
    void rebuild(int upToPos, RebuildCallback done) override
    {
        ++rebuildCount;
        QString s = upToPos>=0 ? script.left(upToPos) : script;
        if (holdRebuild)
        {
            pendingRebuild=done;
            return;
        }
        int e=s.indexOf("Boxx");
        if (e>=0)
            done(false, "unknown command Boxx", e);
        else
            done(true, QString(), -1);
    }
    void cancelRebuild() override { ++cancelCount; pendingRebuild=nullptr; }
    QList<CommandDoc> commandDocumentation() const override
    {
        return {
            {"Box", "( <vector:p0>, <vector:L1>, <vector:L2>, <vector:L3> [, centered] )", "Creates a box."},
            {"Cylinder", "( <vector:p0>, <vector:p1>, <scalar:D> )", "Creates a cylinder."}
        };
    }
    QStringList librarySearchPaths() const override { return {}; }
};


TurnResult toolTurn(const QList<QPair<QString, QJsonObject> >& calls, const QString& text=QString())
{
    TurnResult r;
    r.stopReason="tool_use";
    r.message.role=ChatMessage::Assistant;
    r.message.text=text;
    r.message.providerId="fake";
    static int n=0;
    for (const auto& c: calls)
    {
        ToolCall tc;
        tc.id=QString("call_%1").arg(++n);
        tc.name=c.first;
        tc.input=c.second;
        r.message.toolCalls.append(tc);
    }
    r.usage.inputTokens=100;
    r.usage.outputTokens=10;
    return r;
}


TurnResult textTurn(const QString& text)
{
    TurnResult r;
    r.stopReason="end_turn";
    r.message.role=ChatMessage::Assistant;
    r.message.text=text;
    r.message.providerId="fake";
    return r;
}


/**
 * check, that roles alternate and every tool call is answered by the following message
 */
void checkHistoryConsistency(const QList<ChatMessage>& h)
{
    for (int i=0; i<h.size(); ++i)
    {
        QCOMPARE(int(h[i].role), int(i%2==0 ? ChatMessage::User : ChatMessage::Assistant));
        if (h[i].role==ChatMessage::Assistant && !h[i].toolCalls.isEmpty() && i+1<h.size())
        {
            QCOMPARE(h[i+1].toolResults.size(), h[i].toolCalls.size());
            for (int j=0; j<h[i].toolCalls.size(); ++j)
                QCOMPARE(h[i+1].toolResults[j].toolCallId, h[i].toolCalls[j].id);
        }
    }
}




// ================================================================
// local HTTP server, which answers with prepared responses
// ================================================================

class FakeHttpServer : public QTcpServer
{
public:
    QList<QByteArray> responses;
    QList<QByteArray> requests;

    FakeHttpServer()
    {
        connect(this, &QTcpServer::newConnection, this, [this]()
        {
            while (auto *s=nextPendingConnection())
            {
                auto buf=std::make_shared<QByteArray>();
                connect(s, &QTcpSocket::readyRead, s, [this,s,buf]()
                {
                    buf->append(s->readAll());
                    int he=buf->indexOf("\r\n\r\n");
                    if (he<0) return;
                    QByteArray head=buf->left(he);
                    int cl=0;
                    for (auto l: head.split('\n'))
                        if (l.toLower().startsWith("content-length:"))
                            cl=l.mid(15).trimmed().toInt();
                    if (buf->size()<he+4+cl) return;
                    requests.append(*buf);
                    QByteArray resp = responses.isEmpty() ? QByteArray("HTTP/1.1 500 X\r\nContent-Length: 0\r\n\r\n") : responses.takeFirst();
                    s->write(resp);
                    s->flush();
                    s->disconnectFromHost();
                });
            }
        });
    }
};


QByteArray sseResponse(const QByteArray& body)
{
    return "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nConnection: close\r\n\r\n"+body;
}


const QByteArray sampleStream =
    "event: message_start\n"
    "data: {\"type\":\"message_start\",\"message\":{\"id\":\"msg_1\",\"type\":\"message\",\"role\":\"assistant\",\"model\":\"claude-opus-5-5\",\"content\":[],\"usage\":{\"input_tokens\":12,\"cache_read_input_tokens\":3000,\"cache_creation_input_tokens\":50,\"output_tokens\":1}}}\n\n"
    "event: content_block_start\n"
    "data: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"thinking\",\"thinking\":\"\",\"signature\":\"\"}}\n\n"
    "event: content_block_delta\n"
    "data: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"thinking_delta\",\"thinking\":\"Need to read.\"}}\n\n"
    "event: content_block_delta\n"
    "data: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"signature_delta\",\"signature\":\"SIG123\"}}\n\n"
    "event: content_block_stop\n"
    "data: {\"type\":\"content_block_stop\",\"index\":0}\n\n"
    "event: ping\n"
    "data: {\"type\":\"ping\"}\n\n"
    "event: content_block_start\n"
    "data: {\"type\":\"content_block_start\",\"index\":1,\"content_block\":{\"type\":\"text\",\"text\":\"\"}}\n\n"
    "event: content_block_delta\n"
    "data: {\"type\":\"content_block_delta\",\"index\":1,\"delta\":{\"type\":\"text_delta\",\"text\":\"Let me \"}}\n\n"
    "event: content_block_delta\n"
    "data: {\"type\":\"content_block_delta\",\"index\":1,\"delta\":{\"type\":\"text_delta\",\"text\":\"look.\"}}\n\n"
    "event: content_block_stop\n"
    "data: {\"type\":\"content_block_stop\",\"index\":1}\n\n"
    "event: content_block_start\n"
    "data: {\"type\":\"content_block_start\",\"index\":2,\"content_block\":{\"type\":\"tool_use\",\"id\":\"toolu_1\",\"name\":\"read_script\",\"input\":{}}}\n\n"
    "event: content_block_delta\n"
    "data: {\"type\":\"content_block_delta\",\"index\":2,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"{\\\"from_\"}}\n\n"
    "event: content_block_delta\n"
    "data: {\"type\":\"content_block_delta\",\"index\":2,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"line\\\": 3}\"}}\n\n"
    "event: content_block_stop\n"
    "data: {\"type\":\"content_block_stop\",\"index\":2}\n\n"
    "event: message_delta\n"
    "data: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"tool_use\"},\"usage\":{\"output_tokens\":42}}\n\n"
    "event: message_stop\n"
    "data: {\"type\":\"message_stop\"}\n\n";





QByteArray sseData(const QList<QByteArray>& chunks)
{
    QByteArray r;
    for (const auto& c: chunks)
        r+="data: "+c+"\n\n";
    return r;
}


const QByteArray openaiStream = sseData({
    "{\"id\":\"c1\",\"model\":\"qwen3:32b\",\"choices\":[{\"index\":0,\"delta\":{\"role\":\"assistant\",\"reasoning_content\":\"Think.\"},\"finish_reason\":null}]}",
    "{\"id\":\"c1\",\"model\":\"qwen3:32b\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\"I will \"},\"finish_reason\":null}]}",
    "{\"id\":\"c1\",\"model\":\"qwen3:32b\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\"edit.\"},\"finish_reason\":null}]}",
    "{\"id\":\"c1\",\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_a\",\"type\":\"function\",\"function\":{\"name\":\"edit_script\",\"arguments\":\"\"}}]},\"finish_reason\":null}]}",
    "{\"id\":\"c1\",\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":0,\"function\":{\"arguments\":\"{\\\"old_string\\\":\\\"L=1\"}}]},\"finish_reason\":null}]}",
    "{\"id\":\"c1\",\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":0,\"function\":{\"arguments\":\";\\\",\\\"new_string\\\":\\\"L=2;\\\"}\"}}]},\"finish_reason\":null}]}",
    "{\"id\":\"c1\",\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":1,\"id\":\"call_b\",\"type\":\"function\",\"function\":{\"name\":\"rebuild\",\"arguments\":\"{}\"}}]},\"finish_reason\":null}]}",
    "{\"id\":\"c1\",\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"tool_calls\"}]}",
    "{\"id\":\"c1\",\"choices\":[],\"usage\":{\"prompt_tokens\":1000,\"completion_tokens\":50,\"prompt_tokens_details\":{\"cached_tokens\":800}}}",
    "[DONE]"
});


// ================================================================
// tests
// ================================================================

class ISCADAssistantTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void initTestCase()
    {
        qRegisterMetaType<TurnResult>();
    }


    void sseParserChunked()
    {
        QByteArray stream="event: a\r\ndata: 1\r\n\r\n: comment\nevent: b\ndata: x\ndata: y\n\ndata:z\n\n";
        // feed byte by byte
        SSEStreamParser p;
        QList<SSEStreamParser::Event> evs;
        for (char c: stream)
            evs+=p.feed(QByteArray(1, c));
        evs+=p.finish();
        QCOMPARE(evs.size(), 3);
        QCOMPARE(evs[0].event, QByteArray("a"));
        QCOMPARE(evs[0].data, QByteArray("1"));
        QCOMPARE(evs[1].event, QByteArray("b"));
        QCOMPARE(evs[1].data, QByteArray("x\ny"));
        QCOMPARE(evs[2].event, QByteArray());
        QCOMPARE(evs[2].data, QByteArray("z"));
    }


    void streamAccumulator()
    {
        SSEStreamParser p;
        AnthropicStreamAccumulator acc;
        QString text, thinking;
        acc.onText=[&](const QString& t) { text+=t; };
        acc.onThinking=[&](const QString& t) { thinking+=t; };

        // split at odd positions
        for (int i=0; i<sampleStream.size(); i+=37)
            for (const auto& ev: p.feed(sampleStream.mid(i, 37)))
                acc.processEvent(ev);

        QCOMPARE(int(acc.status()), int(AnthropicStreamAccumulator::Done));
        QCOMPARE(text, QString("Let me look."));
        QCOMPARE(thinking, QString("Need to read."));

        auto r=acc.result("anthropic");
        QCOMPARE(r.stopReason, QString("tool_use"));
        QCOMPARE(r.model, QString("claude-opus-5-5"));
        QCOMPARE(r.usage.outputTokens, qint64(42));
        QCOMPARE(r.usage.cacheReadTokens, qint64(3000));
        QCOMPARE(r.usage.cacheWriteTokens, qint64(50));
        QCOMPARE(r.message.text, QString("Let me look."));
        QCOMPARE(r.message.toolCalls.size(), 1);
        QCOMPARE(r.message.toolCalls[0].id, QString("toolu_1"));
        QCOMPARE(r.message.toolCalls[0].input.value("from_line").toInt(), 3);
        QVERIFY(r.message.toolCalls[0].inputValid);

        // raw content for the history: thinking block with signature, tool input filled in
        auto c=r.message.providerContent.toArray();
        QCOMPARE(c.size(), 3);
        QCOMPARE(c[0].toObject().value("type").toString(), QString("thinking"));
        QCOMPARE(c[0].toObject().value("signature").toString(), QString("SIG123"));
        QCOMPARE(c[2].toObject().value("input").toObject().value("from_line").toInt(), 3);
    }


    void streamAccumulatorInvalidToolInput()
    {
        QByteArray s=
            "data: {\"type\":\"message_start\",\"message\":{\"model\":\"m\",\"usage\":{}}}\n\n"
            "data: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"tool_use\",\"id\":\"t\",\"name\":\"edit_script\",\"input\":{}}}\n\n"
            "data: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"{\\\"old_string\\\": \\\"abc\"}}\n\n"
            "data: {\"type\":\"content_block_stop\",\"index\":0}\n\n"
            "data: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"max_tokens\"},\"usage\":{\"output_tokens\":5}}\n\n"
            "data: {\"type\":\"message_stop\"}\n\n";
        SSEStreamParser p;
        AnthropicStreamAccumulator acc;
        for (const auto& ev: p.feed(s))
            acc.processEvent(ev);
        auto r=acc.result("anthropic");
        QCOMPARE(r.message.toolCalls.size(), 1);
        QVERIFY(!r.message.toolCalls[0].inputValid);
        // the marker must not be sent back to the API
        QVERIFY(!r.message.providerContent.toArray()[0].toObject().contains("__invalid_input"));
    }


    void streamErrorEvent()
    {
        SSEStreamParser p;
        AnthropicStreamAccumulator acc;
        for (const auto& ev: p.feed("event: error\ndata: {\"type\":\"error\",\"error\":{\"type\":\"overloaded_error\",\"message\":\"Overloaded\"}}\n\n"))
            acc.processEvent(ev);
        QCOMPARE(int(acc.status()), int(AnthropicStreamAccumulator::Error));
        QCOMPARE(acc.errorType(), QString("overloaded_error"));
    }


    void requestFormat()
    {
        AnthropicConfig cfg;
        cfg.model="claude-opus-5-5";

        QList<ChatMessage> h;
        ChatMessage u; u.role=ChatMessage::User; u.text="make a box";
        h<<u;

        // assistant message from this backend: echoed unchanged
        ChatMessage a; a.role=ChatMessage::Assistant;
        a.providerId="anthropic";
        a.providerContent=QJsonArray{
            QJsonObject{{"type","thinking"},{"thinking","hmm"},{"signature","S"}},
            QJsonObject{{"type","tool_use"},{"id","toolu_1"},{"name","read_script"},{"input",QJsonObject()}}
        };
        ToolCall tc; tc.id="toolu_1"; tc.name="read_script";
        a.toolCalls<<tc;
        h<<a;

        ChatMessage r; r.role=ChatMessage::User;
        r.toolResults<<ToolResult{"toolu_1", "     1\tx=1;", false};
        r.text="also round the edges";
        h<<r;

        auto req=AnthropicBackend::buildRequest(
            cfg, "anthropic", "SYSTEM", h, ISCADAssistantAgent::toolDefinitions());

        QCOMPARE(req.value("model").toString(), QString("claude-opus-5-5"));
        QVERIFY(req.value("stream").toBool());
        QVERIFY(!req.contains("temperature"));
        QCOMPARE(req.value("thinking").toObject().value("type").toString(), QString("adaptive"));
        QCOMPARE(req.value("output_config").toObject().value("effort").toString(), QString("high"));
        QCOMPARE(req.value("fallbacks").toString(), QString("default"));

        auto sys=req.value("system").toArray();
        QCOMPARE(sys.size(), 1);
        QVERIFY(sys[0].toObject().contains("cache_control"));

        auto tools=req.value("tools").toArray();
        QCOMPARE(tools.size(), ISCADAssistantAgent::toolDefinitions().size());
        QVERIFY(tools[0].toObject().value("eager_input_streaming").toBool());

        auto msgs=req.value("messages").toArray();
        QCOMPARE(msgs.size(), 3);
        auto ac=msgs[1].toObject().value("content").toArray();
        QCOMPARE(ac[0].toObject().value("signature").toString(), QString("S"));

        // tool results come first in the user content, the last block is cached
        auto uc=msgs[2].toObject().value("content").toArray();
        QCOMPARE(uc.size(), 2);
        QCOMPARE(uc[0].toObject().value("type").toString(), QString("tool_result"));
        QCOMPARE(uc[0].toObject().value("tool_use_id").toString(), QString("toolu_1"));
        QCOMPARE(uc[1].toObject().value("type").toString(), QString("text"));
        QVERIFY(uc[1].toObject().contains("cache_control"));
        QVERIFY(!uc[0].toObject().contains("cache_control"));

        // other endpoints: no fallback parameter
        cfg.baseUrl=QUrl("http://localhost:1234");
        auto req2=AnthropicBackend::buildRequest(cfg, "anthropic", "S", h, {});
        QVERIFY(!req2.contains("fallbacks"));
        QVERIFY(!req2.contains("tools"));
    }


    void agentEditAndRebuildLoop()
    {
        FakeHost host;
        host.script="# model\nL=10;\nb: Box(O, L*EX, EY, EZ);\n";
        ISCADAssistantAgent agent(&host);
        auto* be=new FakeBackend;
        agent.setBackend(be);

        be->responses
            << toolTurn({{"read_script", QJsonObject()}})
            << toolTurn({{"edit_script", QJsonObject{{"old_string","L=10;"},{"new_string","L=20;\nD=5;"}}}})
            << toolTurn({{"edit_script", QJsonObject{{"old_string","Box(O"},{"new_string","Boxx(O"}}},
                         {"rebuild", QJsonObject()}})
            << toolTurn({{"edit_script", QJsonObject{{"old_string","Boxx"},{"new_string","Box"}}},
                         {"rebuild", QJsonObject()}})
            << textTurn("Changed L to 20.");

        QSignalSpy finished(&agent, &ISCADAssistantAgent::requestFinished);
        agent.sendUserMessage("make it longer");
        QVERIFY(agent.isRunning());
        QVERIFY(finished.wait(2000));
        QCOMPARE(finished[0][0].toString(), QString("end_turn"));
        QVERIFY(!agent.isRunning());

        QCOMPARE(host.script, QString("# model\nL=20;\nD=5;\nb: Box(O, L*EX, EY, EZ);\n"));
        QCOMPARE(host.beginCount, 1);
        QCOMPARE(host.endCount, 1);
        QCOMPARE(host.editCount, 3);
        QCOMPARE(host.rebuildCount, 2);

        // the first request contains the script
        QVERIFY(be->requests[0][0].text.contains("     2\tL=10;"));
        QVERIFY(be->requests[0][0].text.contains("make it longer"));

        const auto& h=agent.history();
        checkHistoryConsistency(h);
        QCOMPARE(h.size(), 10);
        QVERIFY(h[2].toolResults[0].content.contains("     3\tb: Box"));
        QVERIFY(!h[4].toolResults[0].isError);
        // failed rebuild reports line and column
        QVERIFY(h[6].toolResults[1].isError);
        QVERIFY(h[6].toolResults[1].content.contains("line 4, column 4"));
        QVERIFY(h[6].toolResults[1].content.contains("unknown command Boxx"));
        QVERIFY(!h[8].toolResults[1].isError);

        QCOMPARE(agent.sessionUsage().inputTokens, qint64(400));

        // second request: the user modified the script in between
        host.script+="# user comment\n";
        be->responses << textTurn("ok");
        agent.sendUserMessage("thanks");
        QVERIFY(finished.wait(2000));
        QVERIFY(be->requests.last().last().text.contains("modified the script"));
        QVERIFY(!be->requests.last().last().text.contains("Current script"));
        checkHistoryConsistency(agent.history());
    }


    void agentEditErrors()
    {
        FakeHost host;
        host.script="a=1;\na=1;\n";
        ISCADAssistantAgent agent(&host);
        QList<ToolResult> res;
        auto collect=[&](const ToolResult& r) { res<<r; };

        host.beginAssistantTurn();
        ToolCall tc; tc.id="1"; tc.name="edit_script";
        tc.input={{"old_string","a=1;"},{"new_string","a=2;"}};
        agent.executeTool(tc, collect);
        QVERIFY(res.last().isError);
        QVERIFY(res.last().content.contains("2 times (lines 1, 2)"));

        tc.input={{"old_string","b=1;"},{"new_string","a=2;"}};
        agent.executeTool(tc, collect);
        QVERIFY(res.last().isError);
        QVERIFY(res.last().content.contains("not found"));

        tc.input={{"old_string","a=1;"},{"new_string","a=2;"},{"replace_all",true}};
        agent.executeTool(tc, collect);
        QVERIFY(!res.last().isError);
        QCOMPARE(host.script, QString("a=2;\na=2;\n"));

        tc.name="command_help";
        tc.input={{"name","cylinder"}};
        agent.executeTool(tc, collect);
        QVERIFY(!res.last().isError);
        QVERIFY(res.last().content.startsWith("Cylinder( <vector:p0>"));

        tc.input={{"name","Bo"}};
        agent.executeTool(tc, collect);
        QVERIFY(res.last().isError);
        QVERIFY(res.last().content.contains("Similar commands: Box"));

        tc.name="read_library_model";
        tc.input={{"name","../../etc/passwd"}};
        agent.executeTool(tc, collect);
        QVERIFY(res.last().isError);

        tc.name="nonexistent";
        agent.executeTool(tc, collect);
        QVERIFY(res.last().isError);

        tc.name="edit_script";
        tc.inputValid=false;
        tc.rawInput="{\"old_str";
        agent.executeTool(tc, collect);
        QVERIFY(res.last().isError);
        QVERIFY(res.last().content.contains("Invalid tool input"));
    }


    void agentStopDuringRebuild()
    {
        FakeHost host;
        host.script="x=1;\n";
        host.holdRebuild=true;
        ISCADAssistantAgent agent(&host);
        auto* be=new FakeBackend;
        agent.setBackend(be);
        be->responses << toolTurn({{"rebuild", QJsonObject()}, {"read_script", QJsonObject()}});

        QSignalSpy finished(&agent, &ISCADAssistantAgent::requestFinished);
        agent.sendUserMessage("rebuild");
        QTRY_VERIFY(bool(host.pendingRebuild));

        agent.stop();
        QCOMPARE(finished.size(), 1);
        QCOMPARE(finished[0][0].toString(), QString("stopped"));
        QCOMPARE(host.cancelCount, 1);
        QCOMPARE(host.endCount, 1);
        checkHistoryConsistency(agent.history());
        QVERIFY(agent.history().last().toolResults[0].content.contains("stopped"));

        // the next message continues the conversation with consistent roles
        host.holdRebuild=false;
        be->responses << textTurn("ok");
        agent.sendUserMessage("go on");
        QVERIFY(finished.wait(2000));
        checkHistoryConsistency(agent.history());
        QCOMPARE(agent.history().size(), 4);
        QVERIFY(agent.history()[2].text.contains("go on"));
    }


    void agentBackendFailure()
    {
        FakeHost host;
        ISCADAssistantAgent agent(&host);
        auto* be=new FakeBackend;
        agent.setBackend(be);
        be->failNext=true;

        QSignalSpy finished(&agent, &ISCADAssistantAgent::requestFinished);
        QSignalSpy errors(&agent, &ISCADAssistantAgent::errorOccurred);
        agent.sendUserMessage("hello");
        QVERIFY(finished.wait(2000));
        QCOMPARE(finished[0][0].toString(), QString("error"));
        QCOMPARE(errors.size(), 1);
        // the failed prompt is not kept
        QCOMPARE(agent.history().size(), 0);
        QCOMPARE(host.endCount, 1);
    }


    void agentRefusal()
    {
        FakeHost host;
        ISCADAssistantAgent agent(&host);
        auto* be=new FakeBackend;
        agent.setBackend(be);
        auto r=textTurn("");
        r.stopReason="refusal";
        be->responses << r;

        QSignalSpy finished(&agent, &ISCADAssistantAgent::requestFinished);
        agent.sendUserMessage("something");
        QVERIFY(finished.wait(2000));
        QCOMPARE(finished[0][0].toString(), QString("refusal"));
        QCOMPARE(agent.history().size(), 0);
    }


    void agentStepLimit()
    {
        FakeHost host;
        host.script="x=1;\n";
        ISCADAssistantAgent agent(&host);
        agent.setMaxSteps(3);
        auto* be=new FakeBackend;
        agent.setBackend(be);
        for (int i=0; i<5; ++i)
            be->responses << toolTurn({{"read_script", QJsonObject()}});

        QSignalSpy finished(&agent, &ISCADAssistantAgent::requestFinished);
        agent.sendUserMessage("loop");
        QVERIFY(finished.wait(2000));
        QCOMPARE(finished[0][0].toString(), QString("max_steps"));
        QCOMPARE(be->requests.size(), 3);
        checkHistoryConsistency(agent.history());
    }


    void httpStreamingWithRetry()
    {
        FakeHttpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QByteArray overloaded="{\"type\":\"error\",\"error\":{\"type\":\"overloaded_error\",\"message\":\"Overl.\"}}";
        server.responses
            << QByteArray("HTTP/1.1 529 Overloaded\r\nContent-Type: application/json\r\nretry-after: 1\r\nContent-Length: ")
               +QByteArray::number(overloaded.size())+"\r\n\r\n"+overloaded
            << sseResponse(sampleStream);

        AnthropicConfig cfg;
        cfg.apiKey="sk-test";
        cfg.baseUrl=QUrl(QString("http://127.0.0.1:%1").arg(server.serverPort()));
        AnthropicBackend be(cfg);

        QSignalSpy done(&be, &LLMBackend::turnFinished);
        QSignalSpy failed(&be, &LLMBackend::turnFailed);
        QSignalSpy status(&be, &LLMBackend::statusMessage);
        QSignalSpy text(&be, &LLMBackend::textDelta);

        ChatMessage u; u.role=ChatMessage::User; u.text="hi";
        be.sendTurn("sys", {u}, ISCADAssistantAgent::toolDefinitions());
        QVERIFY(done.wait(10000));
        QCOMPARE(failed.size(), 0);
        QCOMPARE(status.size(), 1); // the retry
        QVERIFY(status[0][0].toString().contains("retrying in 1 s"));
        QCOMPARE(text.size(), 2);

        auto r=done[0][0].value<TurnResult>();
        QCOMPARE(r.message.toolCalls.size(), 1);

        QCOMPARE(server.requests.size(), 2);
        const auto& req=server.requests[1];
        QVERIFY(req.startsWith("POST /v1/messages "));
        QVERIFY(req.toLower().contains("x-api-key: sk-test"));
        QVERIFY(req.toLower().contains("anthropic-version: 2026-10-21"));
        // no fallback beta header for other endpoints
        QVERIFY(!req.toLower().contains("anthropic-beta"));
        auto body=QJsonDocument::fromJson(req.mid(req.indexOf("\r\n\r\n")+4)).object();
        QCOMPARE(body.value("messages").toArray().size(), 1);
    }


    void httpErrorNotRetried()
    {
        FakeHttpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QByteArray err="{\"type\":\"error\",\"error\":{\"type\":\"authentication_error\",\"message\":\"invalid x-api-key\"}}";
        server.responses
            << QByteArray("HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\nContent-Length: ")
               +QByteArray::number(err.size())+"\r\n\r\n"+err;

        AnthropicConfig cfg;
        cfg.apiKey="wrong";
        cfg.baseUrl=QUrl(QString("http://127.0.0.1:%1").arg(server.serverPort()));
        AnthropicBackend be(cfg);
        QSignalSpy failed(&be, &LLMBackend::turnFailed);
        ChatMessage u; u.role=ChatMessage::User; u.text="hi";
        be.sendTurn("sys", {u}, {});
        QVERIFY(failed.wait(5000));
        QCOMPARE(failed[0][0].toString(), QString("HTTP 401, authentication_error: invalid x-api-key"));
        QCOMPARE(server.requests.size(), 1);
        QVERIFY(!be.isBusy());
    }


    void missingApiKey()
    {
        AnthropicConfig cfg;
        AnthropicBackend be(cfg);
        QSignalSpy failed(&be, &LLMBackend::turnFailed);
        be.sendTurn("sys", {}, {});
        QVERIFY(failed.wait(1000));
        QVERIFY(failed[0][0].toString().contains("ANTHROPIC_API_KEY"));
    }


    void openaiStreamAccumulator()
    {
        SSEStreamParser p;
        OpenAICompatStreamAccumulator acc;
        QString text, thinking;
        acc.onText=[&](const QString& t) { text+=t; };
        acc.onThinking=[&](const QString& t) { thinking+=t; };
        for (int i=0; i<openaiStream.size(); i+=23)
            for (const auto& ev: p.feed(openaiStream.mid(i, 23)))
                acc.processEvent(ev);
        QCOMPARE(int(acc.status()), int(StreamAccumulator::Done));
        QCOMPARE(text, QString("I will edit."));
        QCOMPARE(thinking, QString("Think."));

        auto r=acc.result("openai-compat");
        QCOMPARE(r.stopReason, QString("tool_use"));
        QCOMPARE(r.model, QString("qwen3:32b"));
        QCOMPARE(r.usage.inputTokens, qint64(200));
        QCOMPARE(r.usage.cacheReadTokens, qint64(800));
        QCOMPARE(r.usage.outputTokens, qint64(50));
        QCOMPARE(r.message.toolCalls.size(), 2);
        QCOMPARE(r.message.toolCalls[0].id, QString("call_a"));
        QCOMPARE(r.message.toolCalls[0].name, QString("edit_script"));
        QCOMPARE(r.message.toolCalls[0].input.value("new_string").toString(), QString("L=2;"));
        QCOMPARE(r.message.toolCalls[1].name, QString("rebuild"));

        auto pc=r.message.providerContent.toObject();
        QCOMPARE(pc.value("role").toString(), QString("assistant"));
        QCOMPARE(pc.value("content").toString(), QString("I will edit."));
        QCOMPARE(pc.value("tool_calls").toArray().size(), 2);
        // reasoning is not sent back
        QVERIFY(!pc.contains("reasoning_content"));
    }


    void openaiStreamVariants()
    {
        // tool call reported with finish_reason "stop", no index, arguments as object,
        // stream closed without [DONE]
        QByteArray s=sseData({
            "{\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"id\":\"x1\",\"function\":{\"name\":\"read_script\",\"arguments\":{\"from_line\":2}}}]},\"finish_reason\":null}]}",
            "{\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"id\":\"x2\",\"function\":{\"name\":\"rebuild\",\"arguments\":\"\"}}]},\"finish_reason\":null}]}",
            "{\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"stop\"}]}"
        });
        SSEStreamParser p;
        OpenAICompatStreamAccumulator acc;
        for (const auto& ev: p.feed(s))
            acc.processEvent(ev);
        QCOMPARE(int(acc.status()), int(StreamAccumulator::InProgress));
        QCOMPARE(int(acc.finishStream()), int(StreamAccumulator::Done));
        auto r=acc.result("openai-compat");
        QCOMPARE(r.stopReason, QString("tool_use"));
        QCOMPARE(r.message.toolCalls.size(), 2);
        QCOMPARE(r.message.toolCalls[0].input.value("from_line").toInt(), 2);
        QCOMPARE(r.message.toolCalls[1].id, QString("x2"));
        QVERIFY(r.message.toolCalls[1].inputValid);
        // content is null for a pure tool call message
        QVERIFY(r.message.providerContent.toObject().value("content").isNull());

        // invalid arguments, truncated output
        QByteArray s2=sseData({
            "{\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"y\",\"function\":{\"name\":\"edit_script\",\"arguments\":\"{\\\"old_str\"}}]},\"finish_reason\":\"length\"}]}",
            "[DONE]"
        });
        SSEStreamParser p2;
        OpenAICompatStreamAccumulator acc2;
        for (const auto& ev: p2.feed(s2))
            acc2.processEvent(ev);
        auto r2=acc2.result("openai-compat");
        QCOMPARE(r2.stopReason, QString("max_tokens"));
        QVERIFY(!r2.message.toolCalls[0].inputValid);

        // error in stream
        SSEStreamParser p3;
        OpenAICompatStreamAccumulator acc3;
        for (const auto& ev: p3.feed(sseData({"{\"error\":{\"message\":\"boom\",\"type\":\"server_error\"}}"})))
            acc3.processEvent(ev);
        QCOMPARE(int(acc3.status()), int(StreamAccumulator::Error));
        QCOMPARE(acc3.errorMessage(), QString("server_error: boom"));
    }


    void openaiRequestFormat()
    {
        OpenAICompatConfig cfg;
        cfg.model="qwen3:32b";

        QList<ChatMessage> h;
        ChatMessage u; u.role=ChatMessage::User; u.text="make a box";
        h<<u;

        // produced by another backend (Anthropic): converted from text and tool calls
        ChatMessage a; a.role=ChatMessage::Assistant;
        a.providerId="anthropic";
        a.providerContent=QJsonArray{ QJsonObject{{"type","thinking"},{"thinking","x"},{"signature","S"}} };
        a.text="Reading.";
        ToolCall tc; tc.id="toolu_1"; tc.name="read_script"; tc.input=QJsonObject{{"from_line",1}};
        a.toolCalls<<tc;
        h<<a;

        ChatMessage r; r.role=ChatMessage::User;
        r.toolResults<<ToolResult{"toolu_1", "failed", true};
        r.text="go on";
        h<<r;

        auto req=OpenAICompatBackend::buildRequest(cfg, "openai-compat", "SYS", h, ISCADAssistantAgent::toolDefinitions());
        QCOMPARE(req.value("model").toString(), QString("qwen3:32b"));
        QVERIFY(req.value("stream").toBool());
        QVERIFY(req.value("stream_options").toObject().value("include_usage").toBool());
        QVERIFY(!req.contains("max_tokens"));

        auto tools=req.value("tools").toArray();
        QCOMPARE(tools[0].toObject().value("type").toString(), QString("function"));
        QCOMPARE(tools[0].toObject().value("function").toObject().value("name").toString(), QString("read_script"));
        QVERIFY(tools[0].toObject().value("function").toObject().contains("parameters"));

        auto m=req.value("messages").toArray();
        QCOMPARE(m.size(), 5);
        QCOMPARE(m[0].toObject().value("role").toString(), QString("system"));
        QCOMPARE(m[1].toObject().value("role").toString(), QString("user"));
        auto am=m[2].toObject();
        QCOMPARE(am.value("role").toString(), QString("assistant"));
        QCOMPARE(am.value("content").toString(), QString("Reading."));
        auto tcf=am.value("tool_calls").toArray()[0].toObject().value("function").toObject();
        QCOMPARE(tcf.value("arguments").toString(), QString("{\"from_line\":1}"));
        QCOMPARE(m[3].toObject().value("role").toString(), QString("tool"));
        QCOMPARE(m[3].toObject().value("tool_call_id").toString(), QString("toolu_1"));
        QCOMPARE(m[3].toObject().value("content").toString(), QString("ERROR: failed"));
        QCOMPARE(m[4].toObject().value("content").toString(), QString("go on"));

        // and back: an OpenAI message in an Anthropic request
        ChatMessage o; o.role=ChatMessage::Assistant;
        o.providerId="openai-compat";
        o.providerContent=QJsonObject{{"role","assistant"}};
        o.toolCalls<<tc;
        QList<ChatMessage> h2{u, o, r};
        auto areq=AnthropicBackend::buildRequest(AnthropicConfig(), "anthropic", "S", h2, {});
        auto ac=areq.value("messages").toArray()[1].toObject().value("content").toArray();
        QCOMPARE(ac.size(), 1);
        QCOMPARE(ac[0].toObject().value("type").toString(), QString("tool_use"));
        QCOMPARE(ac[0].toObject().value("input").toObject().value("from_line").toInt(), 1);
    }


    void errorBodies()
    {
        QCOMPARE(StreamingHttpBackend::errorMessageFromBody(404, "{\"error\":\"model 'x' not found\"}"),
                 QString("HTTP 404: model 'x' not found"));
        QCOMPARE(StreamingHttpBackend::errorMessageFromBody(400, "{\"error\":{\"message\":\"bad\",\"type\":\"invalid_request_error\"}}"),
                 QString("HTTP 400, invalid_request_error: bad"));
        QCOMPARE(StreamingHttpBackend::errorMessageFromBody(502, ""), QString("HTTP 502"));
        QCOMPARE(StreamingHttpBackend::endpoint(QUrl("http://h:1/v1/"), "/models").toString(),
                 QString("http://h:1/v1/models"));
        QCOMPARE(OpenAICompatBackend::parseModelList("{\"object\":\"list\",\"data\":[{\"id\":\"b\"},{\"id\":\"a\"}]}"),
                 QStringList({"a","b"}));
    }


    void pricing()
    {
        auto p=AnthropicBackend::pricingOf("claude-opus-5-5");
        QVERIFY(p.known);
        Usage u;
        u.inputTokens=1000000; u.outputTokens=100000; u.cacheReadTokens=1000000; u.cacheWriteTokens=1000000;
        // 4 + 2 + 0.2 + 5
        QCOMPARE(p.cost(u), 11.2);
        QVERIFY(!AnthropicBackend::pricingOf("unknown-model").known);

        OpenAICompatConfig c;
        QVERIFY(!OpenAICompatBackend(c).pricing().known);
        c.priceInput=1;
        QVERIFY(OpenAICompatBackend(c).pricing().known);
    }


    void openaiAgentOverHttp()
    {
        FakeHttpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        server.responses
            << QByteArray("HTTP/1.1 503 Busy\r\nContent-Length: 0\r\nretry-after: 1\r\n\r\n")
            << sseResponse(sseData({
                   "{\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"c1\",\"function\":{\"name\":\"edit_script\",\"arguments\":\"{\\\"old_string\\\":\\\"L=1;\\\",\\\"new_string\\\":\\\"L=2;\\\"}\"}}]},\"finish_reason\":\"tool_calls\"}]}",
                   "[DONE]" }))
            << sseResponse(sseData({
                   "{\"choices\":[{\"index\":0,\"delta\":{\"content\":\"Done.\"},\"finish_reason\":\"stop\"}]}",
                   "[DONE]" }));

        OpenAICompatConfig cfg;
        cfg.baseUrl=QUrl(QString("http://127.0.0.1:%1/v1").arg(server.serverPort()));
        cfg.model="local-model";
        cfg.apiKey="secret";

        FakeHost host;
        host.script="L=1;\n";
        ISCADAssistantAgent agent(&host);
        agent.setSystemPrompt("SYS");
        agent.setBackend(new OpenAICompatBackend(cfg));

        QSignalSpy finished(&agent, &ISCADAssistantAgent::requestFinished);
        agent.sendUserMessage("set L to 2");
        QVERIFY(finished.wait(10000));
        QCOMPARE(finished[0][0].toString(), QString("end_turn"));
        QCOMPARE(host.script, QString("L=2;\n"));

        QCOMPARE(server.requests.size(), 3);
        const auto& req=server.requests[2];
        QVERIFY(req.startsWith("POST /v1/chat/completions "));
        QVERIFY(req.toLower().contains("authorization: bearer secret"));
        auto body=QJsonDocument::fromJson(req.mid(req.indexOf("\r\n\r\n")+4)).object();
        auto msgs=body.value("messages").toArray();
        // system, user, assistant (tool call), tool result
        QCOMPARE(msgs.size(), 4);
        QCOMPARE(msgs[3].toObject().value("role").toString(), QString("tool"));
        QCOMPARE(msgs[3].toObject().value("tool_call_id").toString(), QString("c1"));
        checkHistoryConsistency(agent.history());
    }
};


QTEST_GUILESS_MAIN(ISCADAssistantTest)
#include "ISCADAssistantAgent.moc"
