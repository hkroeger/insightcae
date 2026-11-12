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
#include <QTemporaryDir>

#include "assistant/ssestreamparser.h"
#include "assistant/anthropicbackend.h"
#include "assistant/openaicompatbackend.h"
#include "assistant/iscadassistantagent.h"
#include "assistant/evaluation.h"

using namespace insight::assistant;

Q_DECLARE_METATYPE(insight::assistant::TurnResult)
Q_DECLARE_METATYPE(insight::assistant::TaskResult)


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

    SymbolTable symbols() override
    {
        SymbolTable t;
        t.ok=true;
        t.scalars.append({"L", 10.});
        t.scalars.append({"X", std::optional<double>()});
        t.features.append({"b", true, true});
        t.features.append({"tool", false, true});
        t.features.append({"later", false, false});
        return t;
    }
    FeatureProperties featureProperties(const QString& name) override
    {
        FeatureProperties p;
        if (name=="b")
        {
            p.ok=true;
            p.isComponent=true;
            p.nSolids=1; p.nFaces=6;
            p.volume=10.;
            p.surfaceArea=34.;
            p.bbMin=Vec3{0,0,0};
            p.bbMax=Vec3{1,2,5};
            p.cog=Vec3{0.5,1,2.5};
            p.scalars.append({"L1", 1.});
        }
        else if (name=="tool")
        {
            p.ok=true;
            p.volume=3.;
        }
        else
            p.error="Unknown feature";
        return p;
    }
    QString lastDirection, lastHighlight;
    Inspection captureView(const QString& direction, const QString& highlight, QByteArray& png) override
    {
        lastDirection=direction;
        lastHighlight=highlight;
        png=QByteArray("\x89PNG-fake", 9);
        return {true, "View: "+direction};
    }
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


    void agentInspectionTools()
    {
        FakeHost host;
        host.script="b: Box(O, EX, EY, EZ);\n";
        ISCADAssistantAgent agent(&host);
        QList<ToolResult> res;
        auto collect=[&](const ToolResult& r) { res<<r; };
        QSignalSpy images(&agent, &ISCADAssistantAgent::toolImageProduced);

        ToolCall tc; tc.id="1";
        tc.name="list_symbols";
        agent.executeTool(tc, collect);
        QVERIFY(!res.last().isError);
        QVERIFY(res.last().content.contains("L = 10"));
        QVERIFY(res.last().content.contains("X = (not evaluated)"));
        QVERIFY(res.last().content.contains("later (not built)"));

        tc.name="inspect_feature";
        tc.input={{"name","b"}};
        agent.executeTool(tc, collect);
        QVERIFY(!res.last().isError);
        QVERIFY(res.last().content.contains("Volume: 10"));
        QVERIFY(res.last().content.contains("size [1, 2, 5]"));
        QVERIFY(res.last().content.contains("Scalars (b$<name>): L1=1"));

        tc.input={{"name","x"}};
        agent.executeTool(tc, collect);
        QVERIFY(res.last().isError);

        tc.name="view_model";
        tc.input={{"highlight","b"}};
        agent.executeTool(tc, collect);
        QVERIFY(!res.last().isError);
        QCOMPARE(host.lastDirection, QString("iso"));
        QCOMPARE(host.lastHighlight, QString("b"));
        QCOMPARE(res.last().images.size(), 1);
        QCOMPARE(res.last().images[0].mediaType, QString("image/png"));
        QCOMPARE(images.size(), 1);

        tc.input={{"direction","sideways"}};
        agent.executeTool(tc, collect);
        QVERIFY(res.last().isError);

        // disabled for models without image input
        agent.setDisabledTools({"view_model"});
        for (const auto& t: agent.tools())
            QVERIFY(t.name!="view_model");
        QCOMPARE(agent.tools().size(), ISCADAssistantAgent::toolDefinitions().size()-1);
        tc.input={{"direction","top"}};
        agent.executeTool(tc, collect);
        QVERIFY(res.last().isError);
        QVERIFY(res.last().content.contains("not available"));
        QCOMPARE(host.lastDirection, QString("iso"));
    }


    void imagesInRequests()
    {
        ChatMessage u; u.role=ChatMessage::User; u.text="look";
        ChatMessage a; a.role=ChatMessage::Assistant;
        ToolCall tc; tc.id="t1"; tc.name="view_model";
        a.toolCalls<<tc;
        ChatMessage r; r.role=ChatMessage::User;
        ToolResult tr{"t1", "View: iso", false, {ToolImage{"image/png", QByteArray("PNGDATA")}}};
        r.toolResults<<tr;
        r.text="and now?";
        QList<ChatMessage> h{u, a, r};

        // Anthropic: image inside the tool result
        auto areq=AnthropicBackend::buildRequest(AnthropicConfig(), "anthropic", "S", h, {});
        auto ac=areq.value("messages").toArray()[2].toObject().value("content").toArray();
        auto trc=ac[0].toObject().value("content").toArray();
        QCOMPARE(trc.size(), 2);
        QCOMPARE(trc[0].toObject().value("text").toString(), QString("View: iso"));
        auto src=trc[1].toObject().value("source").toObject();
        QCOMPARE(trc[1].toObject().value("type").toString(), QString("image"));
        QCOMPARE(src.value("media_type").toString(), QString("image/png"));
        QCOMPARE(QByteArray::fromBase64(src.value("data").toString().toLatin1()), QByteArray("PNGDATA"));

        // OpenAI compatible: tool message (text) followed by a user message with the image
        OpenAICompatConfig oc; oc.model="m";
        auto oreq=OpenAICompatBackend::buildRequest(oc, "openai-compat", "S", h, {});
        auto om=oreq.value("messages").toArray();
        QCOMPARE(om.size(), 5); // system, user, assistant, tool, user(image+text)
        QCOMPARE(om[3].toObject().value("role").toString(), QString("tool"));
        QVERIFY(om[3].toObject().value("content").toString().contains("1 image(s) attached"));
        auto parts=om[4].toObject().value("content").toArray();
        QCOMPARE(om[4].toObject().value("role").toString(), QString("user"));
        QCOMPARE(parts[1].toObject().value("type").toString(), QString("image_url"));
        QCOMPARE(parts[1].toObject().value("image_url").toObject().value("url").toString(),
                 QString("data:image/png;base64,")+QString::fromLatin1(QByteArray("PNGDATA").toBase64()));
        QCOMPARE(parts.last().toObject().value("text").toString(), QString("and now?"));
    }


    void evalTaskLoading()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        auto write=[&](const QString& fn, const QByteArray& c)
        {
            QFile f(dir.filePath(fn));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(c);
        };
        write("start.iscad", "b: Box(O, EX, EY, EZ);\n");
        write("01_a.json", "{\"id\":\"a\",\"category\":\"create\",\"prompt\":\"make\",\"checks\":[{\"type\":\"builds\"}],"
                           "\"solution\":\"x\",\"start_builds\":false}");
        write("02_b.json", "[{\"id\":\"b\",\"prompt\":\"p\",\"script_file\":\"start.iscad\",\"checks\":[{\"type\":\"builds\"}]},"
                           " {\"id\":\"c\",\"prompt\":\"p\",\"checks\":[{\"type\":\"builds\"}],\"max_steps\":7}]");
        QString err;
        auto tasks=EvalTask::loadDirectory(dir.path(), &err);
        QVERIFY2(err.isEmpty(), qPrintable(err));
        QCOMPARE(tasks.size(), 3);
        QCOMPARE(tasks[0].id, QString("a"));
        QVERIFY(!tasks[0].startBuilds);
        QCOMPARE(tasks[0].solution, QString("x"));
        QCOMPARE(tasks[1].script, QString("b: Box(O, EX, EY, EZ);\n"));
        QCOMPARE(tasks[1].fileName, QString("start.iscad"));
        QCOMPARE(tasks[2].maxSteps, 7);

        write("03_dup.json", "{\"id\":\"a\",\"prompt\":\"p\",\"checks\":[{\"type\":\"builds\"}]}");
        EvalTask::loadDirectory(dir.path(), &err);
        QVERIFY(err.contains("duplicate"));

        err.clear();
        write("03_dup.json", "{\"id\":\"d\",\"prompt\":\"p\"}");
        EvalTask::loadDirectory(dir.path(), &err);
        QVERIFY(err.contains("no checks"));
    }


    void evalChecks()
    {
        FakeHost host;
        host.script="L = 10;\nb: Box(O, EX, 2*EY, 5*EZ);\n";
        EvalContext ctx;
        ctx.host=&host;
        ctx.initialScript=host.script;
        ctx.finalBuildOk=true;
        ctx.finalAnswer="The volume is 1,234.5 mm³, i.e. about 1.23 cm³.";

        auto check=[&](const QByteArray& json)
        {
            auto r=evaluateChecks(QJsonArray{QJsonDocument::fromJson(json).object()}, ctx);
            return r.first();
        };

        QVERIFY(check("{\"type\":\"builds\"}").passed);
        QVERIFY(check("{\"type\":\"script_contains\",\"text\":\"Box(\"}").passed);
        QVERIFY(!check("{\"type\":\"script_not_contains\",\"text\":\"Box(\"}").passed);
        QVERIFY(check("{\"type\":\"script_matches\",\"regex\":\"^\\\\s*L\\\\s*=\\\\s*10;\"}").passed);
        QVERIFY(check("{\"type\":\"script_unchanged\"}").passed);
        QVERIFY(check("{\"type\":\"scalar\",\"name\":\"L\",\"value\":10.01}").passed);
        QVERIFY(!check("{\"type\":\"scalar\",\"name\":\"L\",\"value\":11}").passed);
        QVERIFY(check("{\"type\":\"scalar\",\"name\":\"X\",\"value\":1}").message.contains("not evaluated"));
        QVERIFY(check("{\"type\":\"scalar\",\"name\":\"Q\",\"value\":1}").message.contains("not found"));

        QVERIFY(check("{\"type\":\"feature\",\"feature\":\"b\",\"volume\":10,\"bbox_size\":[1,2,5],"
                      "\"bbox_min\":[0,0,0],\"cog\":[0.5,1,2.5],\"n_solids\":1,\"n_faces\":6,\"area\":34}").passed);
        auto r=check("{\"type\":\"feature\",\"feature\":\"b\",\"volume\":12,\"bbox_size\":[1,2,6]}");
        QVERIFY(!r.passed);
        QVERIFY(r.message.contains("volume is 10, expected 12"));
        QVERIFY(r.message.contains("bbox_size is [1, 2, 5], expected [1, 2, 6]"));
        QVERIFY(check("{\"type\":\"feature\",\"feature\":\"b\",\"volume\":10.04,\"rel_tol\":0.01}").passed);
        QVERIFY(!check("{\"type\":\"feature\",\"feature\":\"b\",\"volume\":10.5,\"rel_tol\":0.01}").passed);
        QVERIFY(!check("{\"type\":\"feature\",\"feature\":\"nothere\",\"volume\":1}").passed);

        // any feature
        r=check("{\"type\":\"feature\",\"feature\":\"*\",\"volume\":3}");
        QVERIFY(r.passed);
        QCOMPARE(r.message, QString("matched by tool"));
        QVERIFY(!check("{\"type\":\"feature\",\"feature\":\"*\",\"component\":true,\"volume\":3}").passed);
        QVERIFY(check("{\"type\":\"feature_count\",\"min\":2,\"max\":2}").passed);
        QVERIFY(check("{\"type\":\"feature_count\",\"component\":true,\"min\":1,\"max\":1}").passed);

        QVERIFY(check("{\"type\":\"answer_number\",\"value\":1234.5}").passed);
        QVERIFY(check("{\"type\":\"answer_number\",\"value\":1.234,\"rel_tol\":0.01}").passed);
        QVERIFY(!check("{\"type\":\"answer_number\",\"value\":5}").passed);
        QVERIFY(check("{\"type\":\"answer_contains\",\"text\":\"VOLUME\"}").passed);
        QVERIFY(!check("{\"type\":\"unknown_type\"}").passed);

        r=check("{\"type\":\"builds\",\"description\":\"must build\"}");
        QCOMPARE(r.description, QString("must build"));
        ctx.finalBuildOk=false;
        ctx.finalBuildMessage="boom";
        QVERIFY(check("{\"type\":\"builds\"}").message.contains("boom"));

        host.script+="#x\n";
        QVERIFY(!check("{\"type\":\"script_unchanged\"}").passed);
    }


    void evalRunner()
    {
        qRegisterMetaType<TaskResult>();

        EvalTask t1;
        t1.id="edit"; t1.category="modify";
        t1.prompt="change L";
        t1.script="L = 10;\n";
        t1.checks=QJsonArray{
            QJsonObject{{"type","builds"}},
            QJsonObject{{"type","script_contains"},{"text","L = 20;"}}
        };
        t1.solution="L = 20;\n";

        EvalTask t2;
        t2.id="broken"; t2.category="fix";
        t2.prompt="fix";
        t2.script="b: Boxx();\n";
        t2.startBuilds=false;
        t2.checks=QJsonArray{ QJsonObject{{"type","builds"}} };
        t2.solution="b: Boxx();\n"; // wrong on purpose

        // the scripted model edits t1 and gives up on t2
        int backendCalls=0;
        EvalRunner::BackendFactory bf = [&backendCalls]()
        {
            auto* be=new FakeBackend;
            if (backendCalls++==0)
            {
                be->responses
                    << toolTurn({{"edit_script", QJsonObject{{"old_string","L = 10;"},{"new_string","L = 20;"}}}})
                    << textTurn("Done, L is 20.");
            }
            else
            {
                be->responses << textTurn("I cannot fix it.");
            }
            return be;
        };
        EvalRunner::HostFactory hf = [](const EvalTask& t)
        {
            auto* h=new FakeHost;
            h->script=t.script;
            return h;
        };
        EvalRunner::Config cfg;
        cfg.systemPrompt="SYS";
        cfg.disabledTools=QStringList{"view_model"};
        cfg.timeoutSeconds=10;
        EvalRunner runner(hf, bf, cfg);

        QSignalSpy finished(&runner, &EvalRunner::taskFinished);
        auto results=runner.run({t1, t2});
        QCOMPARE(results.size(), 2);
        QCOMPARE(finished.size(), 2);

        const auto& r1=results[0];
        QVERIFY(r1.passed);
        QCOMPARE(r1.finishReason, QString("end_turn"));
        QCOMPARE(r1.modelResponses, 2);
        QCOMPARE(r1.toolCalls, 1);
        QCOMPARE(r1.finalScript, QString("L = 20;\n"));
        QCOMPARE(r1.finalAnswer, QString("Done, L is 20."));
        QCOMPARE(r1.transcript.size(), 4);

        const auto& r2=results[1];
        QVERIFY(!r2.passed);
        QVERIFY(r2.checks[0].message.contains("unknown command Boxx"));

        auto rep=EvalRunner::report(results, QJsonObject{{"model","fake"}});
        QCOMPARE(rep.value("summary").toObject().value("passed").toInt(), 1);
        QCOMPARE(rep.value("results").toArray().size(), 2);
        QVERIFY(!rep.value("summary").toObject().contains("cost_usd")); // unknown pricing

        auto md=EvalRunner::markdownReport(results, QJsonObject{{"model","fake"}});
        QVERIFY(md.contains("**Passed 1 of 2 (50 %)**"));
        QVERIFY(md.contains("| modify | 1/1 |"));
        QVERIFY(md.contains("| broken | 1 | **FAIL** |"));

        // validation of the task definitions with the reference solutions
        QVERIFY(runner.validateTask(t1).isEmpty());
        auto pr=runner.validateTask(t2);
        QCOMPARE(pr.size(), 1);
        QVERIFY(pr[0].startsWith("solution fails check builds"));
        t2.startBuilds=true;
        QVERIFY(runner.validateTask(t2).first().startsWith("the initial script does not build"));
    }


    void evalTimeout()
    {
        EvalTask t;
        t.id="slow"; t.prompt="p";
        t.checks=QJsonArray{ QJsonObject{{"type","builds"}} };
        EvalRunner::Config cfg;
        cfg.timeoutSeconds=1;
        EvalRunner runner(
            [](const EvalTask&) { return new FakeHost; },
            []() { auto* b=new FakeBackend; b->holdResponse=true; return b; },
            cfg );
        auto r=runner.runTask(t);
        QCOMPARE(r.finishReason, QString("timeout"));
        QVERIFY(!r.passed || r.checks.size()==1);
    }


    void basicToolSet()
    {
        auto disabled=ISCADAssistantAgent::toolsNotIn(ISCADAssistantAgent::basicToolSet());
        QVERIFY(disabled.contains("view_model"));
        QVERIFY(disabled.contains("list_library_models"));
        QVERIFY(!disabled.contains("edit_script"));
        QCOMPARE(disabled.size()+ISCADAssistantAgent::basicToolSet().size(),
                 ISCADAssistantAgent::toolDefinitions().size());
    }
};


QTEST_GUILESS_MAIN(ISCADAssistantTest)
#include "ISCADAssistantAgent.moc"
