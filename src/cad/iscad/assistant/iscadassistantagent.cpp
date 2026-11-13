#include "iscadassistantagent.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QJsonArray>
#include <QTextStream>

namespace insight {
namespace assistant {


namespace {


const int maxReadChars = 200000;


QJsonObject schema(const QJsonObject& properties, const QStringList& required = {})
{
    QJsonObject s{
        {"type", "object"},
        {"properties", properties},
        {"additionalProperties", false}
    };
    if (!required.isEmpty())
        s["required"]=QJsonArray::fromStringList(required);
    return s;
}


QJsonObject prop(const QString& type, const QString& description)
{
    return QJsonObject{ {"type", type}, {"description", description} };
}


/**
 * start positions of all lines
 */
QList<int> lineStarts(const QString& text)
{
    QList<int> starts{0};
    for (int i=0; i<text.size(); ++i)
        if (text[i]==QChar('\n'))
            starts.append(i+1);
    return starts;
}


/**
 * 1-based line number of a character position
 */
int lineOf(const QList<int>& starts, int pos)
{
    auto it=std::upper_bound(starts.begin(), starts.end(), pos);
    return int(it-starts.begin());
}


QString numberedLines(const QString& text, int fromLine, int toLine)
{
    QStringList lines=text.split('\n');
    fromLine=std::max(1, fromLine);
    toLine=std::min(int(lines.size()), toLine);
    QString out;
    for (int l=fromLine; l<=toLine; ++l)
    {
        out+=QString("%1\t%2\n").arg(l, 6).arg(lines[l-1]);
    }
    return out;
}


int countOccurrences(const QString& text, const QString& s, QList<int>* positions=nullptr)
{
    int n=0;
    int i=text.indexOf(s);
    while (i>=0)
    {
        ++n;
        if (positions) positions->append(i);
        i=text.indexOf(s, i+std::max(1, int(s.size())));
    }
    return n;
}


QString truncated(const QString& s, int n)
{
    if (s.size()<=n)
        return s;
    return s.left(n)+QString("\n[... truncated, %1 more characters]").arg(s.size()-n);
}


QString oneLine(const QString& s, int n=80)
{
    QString r=s.simplified();
    if (r.size()>n)
        r=r.left(n-3)+"...";
    return r;
}


QString num(double v)
{
    return QString::number(v, 'g', 6);
}


QString vec(const ISCADAssistantHost::Vec3& v)
{
    return QString("[%1, %2, %3]").arg(num(v.x), num(v.y), num(v.z));
}


} // anonymous namespace




ISCADAssistantHost::SymbolTable ISCADAssistantHost::symbols()
{
    SymbolTable t;
    t.error="Not available in this editor.";
    return t;
}


ISCADAssistantHost::FeatureProperties ISCADAssistantHost::featureProperties(const QString&)
{
    FeatureProperties p;
    p.error="Not available in this editor.";
    return p;
}


ISCADAssistantHost::Inspection ISCADAssistantHost::captureView(
    const QString&, const QString&, QByteArray& )
{
    return {false, "Not available in this editor."};
}




QString ISCADAssistantAgent::formatSymbols(const ISCADAssistantHost::SymbolTable& t)
{
    if (!t.ok)
        return t.error;

    QString r;
    if (!t.scalars.isEmpty())
    {
        r+="Scalars:\n";
        for (const auto& s: t.scalars)
            r+=QString("  %1 = %2\n").arg(s.first, s.second ? num(*s.second) : QString("(not evaluated)"));
    }
    auto vecs = [&r](const QString& title, const QList<QPair<QString, std::optional<ISCADAssistantHost::Vec3> > >& l)
    {
        if (l.isEmpty()) return;
        r+=title+":\n";
        for (const auto& v: l)
            r+=QString("  %1 = %2\n").arg(v.first, v.second ? vec(*v.second) : QString("(not evaluated)"));
    };
    vecs("Points", t.points);
    vecs("Directions", t.directions);
    if (!t.datums.isEmpty())
        r+="Datums: "+t.datums.join(", ")+"\n";
    r+="Features:\n";
    if (t.features.isEmpty())
        r+="  (none)\n";
    for (const auto& f: t.features)
    {
        r+="  "+f.name;
        if (f.isComponent) r+=" (component)";
        if (!f.built) r+=" (not built)";
        r+="\n";
    }
    return r;
}


QString ISCADAssistantAgent::formatFeatureProperties(
    const QString& name, const ISCADAssistantHost::FeatureProperties& p )
{
    if (!p.ok)
        return p.error;

    QString r=QString("Feature %1%2\n").arg(name, p.isComponent ? " (component)" : "");
    r+=QString("Topology: %1 solids, %2 shells, %3 faces, %4 edges, %5 vertices\n")
         .arg(p.nSolids).arg(p.nShells).arg(p.nFaces).arg(p.nEdges).arg(p.nVertices);
    r+="Volume: "+(p.volume ? num(*p.volume) : QString("(not available)"))+"\n";
    r+="Surface area: "+(p.surfaceArea ? num(*p.surfaceArea) : QString("(not available)"))+"\n";
    if (p.bbMin && p.bbMax)
    {
        ISCADAssistantHost::Vec3 sz{p.bbMax->x-p.bbMin->x, p.bbMax->y-p.bbMin->y, p.bbMax->z-p.bbMin->z};
        r+=QString("Bounding box: min %1, max %2, size %3\n").arg(vec(*p.bbMin), vec(*p.bbMax), vec(sz));
    }
    if (p.cog)
        r+=QString("Center of gravity%1: %2\n").arg(p.cogIsSurface ? " of surface" : "", vec(*p.cog));
    if (!p.subfeatures.isEmpty())
        r+=QString("Sub-features (%1.<name>): %2\n").arg(name, p.subfeatures.join(", "));
    QStringList l;
    for (const auto& s: p.scalars)
        l<<QString("%1=%2").arg(s.first, num(s.second));
    if (!l.isEmpty())
        r+=QString("Scalars (%1$<name>): %2\n").arg(name, l.join(", "));
    l.clear();
    for (const auto& s: p.points)
        l<<QString("%1=%2").arg(s.first, vec(s.second));
    if (!l.isEmpty())
        r+=QString("Points (%1@<name>): %2\n").arg(name, l.join(", "));
    l.clear();
    for (const auto& s: p.vectors)
        l<<QString("%1=%2").arg(s.first, vec(s.second));
    if (!l.isEmpty())
        r+=QString("Vectors (%1!<name>): %2\n").arg(name, l.join(", "));
    if (!p.datums.isEmpty())
        r+=QString("Datums (%1%<name>): %2\n").arg(name, p.datums.join(", "));
    return r;
}


QStringList ISCADAssistantAgent::basicToolSet()
{
    return {"read_script", "edit_script", "write_script", "rebuild", "command_help", "inspect_feature",
            "read_reference"};
}


QStringList ISCADAssistantAgent::toolsNotIn(const QStringList& toolSet)
{
    QStringList r;
    for (const auto& t: toolDefinitions())
        if (!toolSet.contains(t.name))
            r<<t.name;
    return r;
}


ISCADAssistantAgent::ISCADAssistantAgent(ISCADAssistantHost* host, QObject* parent)
    : QObject(parent),
      host_(host)
{
    updateTools();
}


ISCADAssistantAgent::~ISCADAssistantAgent()
{
    if (running_)
        stop();
}


void ISCADAssistantAgent::setBackend(LLMBackend* backend)
{
    if (running_)
        stop();

    if (backend_)
        backend_->deleteLater();

    backend_=backend;

    if (backend_)
    {
        backend_->setParent(this);
        connect(backend_, &LLMBackend::textDelta,
                this, &ISCADAssistantAgent::assistantTextDelta);
        connect(backend_, &LLMBackend::thinkingDelta,
                this, &ISCADAssistantAgent::assistantThinkingDelta);
        connect(backend_, &LLMBackend::statusMessage,
                this, &ISCADAssistantAgent::statusMessage);
        connect(backend_, &LLMBackend::turnFinished,
                this, &ISCADAssistantAgent::onTurnFinished);
        connect(backend_, &LLMBackend::turnFailed,
                this, &ISCADAssistantAgent::onTurnFailed);
    }
}


void ISCADAssistantAgent::setDisabledTools(const QStringList& names)
{
    disabledTools_=names;
    updateTools();
}


void ISCADAssistantAgent::setReferenceIndex(std::shared_ptr<const ReferenceIndex> index)
{
    referenceIndex_ = (index && !index->isEmpty()) ? index : nullptr;
    updateTools();
}


void ISCADAssistantAgent::updateTools()
{
    tools_.clear();
    for (const auto& t: toolDefinitions())
    {
        if (disabledTools_.contains(t.name))
            continue;
        if (t.name=="read_reference" && !referenceIndex_)
            continue;
        tools_.append(t);
    }
}


void ISCADAssistantAgent::setSystemPrompt(const QString& systemPrompt)
{
    systemPrompt_=systemPrompt;
}


QString ISCADAssistantAgent::defaultSystemPrompt(const QString& languageReference)
{
    QString p=
R"(You are the modeling assistant built into ISCAD, the script-based CAD editor of InsightCAE.
The user sees the model script in an editor next to this chat and the built geometry in a 3D view.
You read and change the script with the provided tools, in the same way a software engineer edits source code.

How to work:
- Read the relevant part of the script before you change it. The script may have been modified by the user since you last read it.
- Make targeted changes with edit_script. Keep the user's naming, structure, formatting and comments. Use write_script only for a new model or when the user asks for a complete rewrite.
- Prefer named parameters for dimensions (e.g. `D_hole = 8;`) over literal numbers in feature calls. Use `?=` for values, which are meant to be overridden when the model is loaded as a submodel.
- Lengths are in the units used by the script (usually millimeters). Angles are in radians; write degrees as `30*deg`.
- After changing the script, call rebuild. If it fails, read the error, fix the script and rebuild again until the model builds. Do not stop with a broken script, unless you cannot fix it; then explain the problem.
- After a successful rebuild, verify the result where it matters: inspect_feature gives dimensions (bounding box), volume and the named values of a feature; view_model (if available) shows the geometry as an image. Do not claim a shape is correct without checking it.
- If you are unsure about a command or its arguments, call command_help. The library models (list_library_models, read_library_model) are examples of good scripts.
- rebuild does not execute postprocessing actions, which write files (exports, drawings, meshes).
- The user can revert all of your edits of one request with a single undo (Ctrl+Z) in the editor.
- Answer in the language of the user. Keep answers short: say what you changed and anything the user should check. Do not repeat the whole script in the chat.
- If a request is ambiguous in a way that matters for the geometry (e.g. which face, which direction), ask before editing.
)";

    if (!languageReference.isEmpty())
    {
        p+="\n\n# ISCAD language reference\n\n";
        p+=languageReference;
    }

    return p;
}


QList<ToolDefinition> ISCADAssistantAgent::toolDefinitions()
{
    return {
        {
            "read_script",
            "Read the current model script from the editor, with line numbers (line number, tab, line contents). "
            "Optionally restrict to a range of lines.",
            schema({
                {"from_line", prop("integer", "first line to read (1-based, default: 1)")},
                {"to_line", prop("integer", "last line to read (inclusive, default: last line)")}
            })
        },
        {
            "edit_script",
            "Replace text in the model script. old_string must match the script exactly "
            "(including whitespace and line breaks, without the line number prefixes of read_script) "
            "and must be unique, unless replace_all is true. Include enough surrounding text to make it unique. "
            "To insert text, include a neighbouring line in old_string and repeat it in new_string.",
            schema({
                {"old_string", prop("string", "the exact text to replace")},
                {"new_string", prop("string", "the replacement text")},
                {"replace_all", prop("boolean", "replace all occurrences (default: false)")}
            }, {"old_string", "new_string"})
        },
        {
            "write_script",
            "Replace the complete model script. Only for new models or complete rewrites requested by the user; "
            "otherwise use edit_script.",
            schema({
                {"content", prop("string", "the complete new script")}
            }, {"content"})
        },
        {
            "rebuild",
            "Parse and build the model from the current script and report errors with line and column. "
            "Postprocessing actions, which write files, are not executed. Can take a while for complex models.",
            schema({
                {"up_to_line", prop("integer", "build only the script up to and including this line (default: everything)")}
            })
        },
        {
            "list_commands",
            "List the available feature commands with their signatures.",
            schema({
                {"filter", prop("string", "only commands containing this text (case insensitive)")}
            })
        },
        {
            "command_help",
            "Show the signature and documentation of a feature command.",
            schema({
                {"name", prop("string", "the command name, e.g. Extrusion")}
            }, {"name"})
        },
        {
            "read_reference",
            "Read a section of the ISCAD language documentation, e.g. about the syntax of selections, "
            "transformations or postprocessing actions. The section titles are listed in the table of contents "
            "of the system prompt. Single feature commands are explained by command_help.",
            schema({
                {"section", prop("string", "section title from the table of contents, e.g. \"Edges\" or \"Transformation\"")}
            }, {"section"})
        },
        {
            "list_symbols",
            "List the symbols of the model as parsed/built by the last rebuild: "
            "scalars and vectors with their values, datums, and features (with their build state). "
            "Call rebuild first, if the script was changed.",
            schema({})
        },
        {
            "inspect_feature",
            "Properties of a built feature: topology (number of solids, faces, edges, vertices), "
            "volume, surface area, bounding box, center of gravity, its sub-features and its named "
            "scalars/points/vectors/datums (accessible in the script as name$x, name@p, name!v, name%d). "
            "Call rebuild first, if the script was changed.",
            schema({
                {"name", prop("string", "feature symbol, optionally with sub-feature path, e.g. \"body\" or \"asm.part1\"")}
            }, {"name"})
        },
        {
            "view_model",
            "Render the 3D view of the built model to an image and look at it. "
            "Use it to check shapes and positions after a rebuild. "
            "The user's own view is not changed.",
            schema({
                {"direction", QJsonObject{
                     {"type", "string"},
                     {"enum", QJsonArray{"iso", "front", "back", "top", "bottom", "left", "right", "current"}},
                     {"description", "view direction: front looks along +Y, top along -Z, left along +X, "
                                     "iso from (+X,-Y,+Z); current: the user's view (default: iso)"}
                 }},
                {"highlight", prop("string", "a feature to highlight (optional)")}
            })
        },
        {
            "list_library_models",
            "List the models of the ISCAD model library (examples, standard parts).",
            schema({
                {"filter", prop("string", "only models whose name contains this text (case insensitive)")}
            })
        },
        {
            "read_library_model",
            "Read the script of a library model.",
            schema({
                {"name", prop("string", "model name as returned by list_library_models, without .iscad")}
            }, {"name"})
        }
    };
}


void ISCADAssistantAgent::resolveOpenToolCalls(const QString& reason)
{
    if (history_.isEmpty())
        return;

    if (history_.last().role==ChatMessage::Assistant
        && !history_.last().toolCalls.isEmpty())
    {
        ChatMessage r;
        r.role=ChatMessage::User;
        for (const auto& tc: history_.last().toolCalls)
            r.toolResults.append({tc.id, reason, true});
        history_.append(r);
    }
}


void ISCADAssistantAgent::sendUserMessage(const QString& text)
{
    if (running_ || text.trimmed().isEmpty())
        return;

    if (!backend_)
    {
        Q_EMIT errorOccurred("No language model configured.");
        return;
    }

    resolveOpenToolCalls("Not executed: the request was interrupted by the user.");

    QString content=text;

    QString script=host_->scriptText();
    bool firstRequest=true;
    for (const auto& m: history_)
        if (m.role==ChatMessage::Assistant)
            firstRequest=false;

    if (firstRequest)
    {
        QString fn=host_->scriptFileName();
        QString ctx=QString("[Editor state: %1, %2 lines.]\n")
                .arg(fn.isEmpty() ? QString("unsaved script") : QString("file %1").arg(fn))
                .arg(script.isEmpty() ? 0 : script.count('\n')+1);
        if (script.trimmed().isEmpty())
        {
            ctx+="[The script is empty.]\n";
        }
        else if (script.size()<=60000)
        {
            ctx+="[Current script:]\n"+numberedLines(script, 1, script.count('\n')+1);
        }
        else
        {
            ctx+="[The script is long; use read_script to read the relevant parts.]\n";
        }
        content=ctx+"\n"+text;
    }
    else if (script!=lastKnownScript_)
    {
        content="[The user has modified the script since your last response. Re-read the relevant parts before editing.]\n\n"+text;
    }

    if (!history_.isEmpty() && history_.last().role==ChatMessage::User)
    {
        // keep alternating roles: append to the pending tool results
        auto& m=history_.last();
        m.text = m.text.isEmpty() ? content : m.text+"\n\n"+content;
    }
    else
    {
        ChatMessage m;
        m.role=ChatMessage::User;
        m.text=content;
        history_.append(m);
    }

    ++runId_;
    running_=true;
    maxPromptTokens_=0;
    contextWarned_=false;
    step_=0;
    host_->beginAssistantTurn();
    Q_EMIT requestStarted();

    requestNextResponse();
}


void ISCADAssistantAgent::requestNextResponse()
{
    if (++step_ > maxSteps_)
    {
        Q_EMIT errorOccurred(
            QString("Stopped after %1 steps. Send a message to let the assistant continue.").arg(maxSteps_));
        finishRequest("max_steps");
        return;
    }
    backend_->sendTurn(systemPrompt_, history_, tools_);
}


void ISCADAssistantAgent::onTurnFinished(const TurnResult& result)
{
    if (!running_)
        return;

    sessionUsage_+=result.usage;
    Q_EMIT usageUpdated(sessionUsage_);
    checkContextUsage(result);

    if (result.stopReason=="refusal")
    {
        lastPromptTokens_=0; // the history is shortened
        // do not keep the declined response, the conversation can continue without it.
        // A pure prompt is removed as well (it would be declined again).
        if (!history_.isEmpty()
            && history_.last().role==ChatMessage::User
            && history_.last().toolResults.isEmpty())
        {
            history_.removeLast();
        }
        Q_EMIT errorOccurred("The model declined to answer this request.");
        finishRequest("refusal");
        return;
    }

    history_.append(result.message);
    Q_EMIT assistantMessageCompleted(result.message.text);

    if (!result.message.toolCalls.isEmpty())
    {
        pendingCalls_=result.message.toolCalls;
        pendingResults_.clear();

        if (result.stopReason=="max_tokens")
        {
            // the tool input might be truncated
            for (const auto& tc: pendingCalls_)
            {
                pendingResults_.append({
                    tc.id,
                    "Not executed: the response was cut off at the output token limit, "
                    "the tool input may be incomplete. Make smaller changes per tool call.",
                    true });
            }
            pendingCalls_.clear();
        }

        executeNextToolCall();
        return;
    }

    if (result.stopReason=="max_tokens")
        Q_EMIT errorOccurred("The response was cut off at the output token limit.");

    finishRequest(result.stopReason.isEmpty() ? QString("end_turn") : result.stopReason);
}


void ISCADAssistantAgent::executeNextToolCall()
{
    if (!running_)
        return;

    if (pendingResults_.size() < pendingCalls_.size())
    {
        const ToolCall tc=pendingCalls_[pendingResults_.size()];
        quint64 id=runId_;

        QString desc=tc.name;
        if (tc.name=="edit_script")
            desc+=": "+oneLine(tc.input.value("old_string").toString(), 60);
        else if (tc.name=="command_help" || tc.name=="read_library_model"
                 || tc.name=="inspect_feature")
            desc+=": "+tc.input.value("name").toString();
        else if (tc.name=="read_reference")
            desc+=": "+tc.input.value("section").toString();
        else if (tc.name=="view_model")
            desc+=": "+tc.input.value("direction").toString("iso");
        Q_EMIT toolCallStarted(tc.name, desc);

        executeTool(tc, [this, id, tc](const ToolResult& r)
        {
            if (id!=runId_ || !running_)
                return;
            ToolResult res=r;
            res.toolCallId=tc.id;
            pendingResults_.append(res);
            Q_EMIT toolCallFinished(tc.name, oneLine(res.content, 120), res.isError);
            executeNextToolCall();
        });
        return;
    }

    // all calls done: return the results
    ChatMessage m;
    m.role=ChatMessage::User;
    m.toolResults=pendingResults_;
    history_.append(m);
    pendingCalls_.clear();
    pendingResults_.clear();

    requestNextResponse();
}


void ISCADAssistantAgent::onTurnFailed(const QString& msg)
{
    if (!running_)
        return;

    Q_EMIT errorOccurred(msg);

    // a pure prompt is removed (the user may send it again),
    // tool results are kept (the edits have been made)
    if (!history_.isEmpty()
        && history_.last().role==ChatMessage::User
        && history_.last().toolResults.isEmpty())
    {
        history_.removeLast();
    }

    finishRequest("error");
}


void ISCADAssistantAgent::finishRequest(const QString& reason)
{
    if (!running_)
        return;

    running_=false;
    ++runId_;
    pendingCalls_.clear();
    pendingResults_.clear();
    if (rebuildRunning_)
    {
        rebuildRunning_=false;
        host_->cancelRebuild();
    }
    host_->endAssistantTurn();
    lastKnownScript_=host_->scriptText();
    Q_EMIT requestFinished(reason);
}


void ISCADAssistantAgent::stop()
{
    if (!running_)
        return;

    if (backend_)
        backend_->cancel();

    // results of the already executed tool calls are kept
    if (!pendingCalls_.isEmpty())
    {
        auto results=pendingResults_;
        for (int i=results.size(); i<pendingCalls_.size(); ++i)
            results.append({pendingCalls_[i].id, "Not executed: stopped by the user.", true});
        ChatMessage m;
        m.role=ChatMessage::User;
        m.toolResults=results;
        history_.append(m);
    }

    finishRequest("stopped");
}


void ISCADAssistantAgent::clearConversation()
{
    stop();
    history_.clear();
    lastKnownScript_.clear();
    lastPromptTokens_=maxPromptTokens_=0;
    truncationSeen_=false;
}


void ISCADAssistantAgent::checkContextUsage(const TurnResult& turn)
{
    const auto& u=turn.usage;
    qint64 p=u.inputTokens+u.cacheReadTokens+u.cacheWriteTokens;
    if (p<=0)
        return; // not reported by the server

    maxPromptTokens_=std::max(maxPromptTokens_, p);
    int cl = backend_ ? backend_->contextLength() : 0;

    const QString remedy=
        "Start a new conversation, choose a shorter reference in the assistant settings "
        "(\"System prompt\") or increase the context window of the server "
        "(Ollama: environment variable OLLAMA_CONTEXT_LENGTH).";

    QString w;
    if (lastPromptTokens_>0 && p<lastPromptTokens_ && !turn.alternativeRequest)
    {
        // (an alternative request format, e.g. tools in the text, may be shorter)
        // the conversation only grows: a shorter input means, that the server dropped a part
        truncationSeen_=true;
        w=QString("The input of the model was shorter than before (%1 instead of at least %2 tokens): "
                  "the server has probably cut off parts of the conversation or of the instructions "
                  "to fit its context window%3. The results may be unreliable. ")
              .arg(p).arg(lastPromptTokens_)
              .arg(cl>0 ? QString(" of %1 tokens").arg(cl) : QString())
          +remedy;
    }
    else if (truncationSeen_)
    {
        w=QString("The conversation (%1 tokens) does not fit into the context window of the model: "
                  "parts of it are cut off by the server. ").arg(p)+remedy;
    }
    else if (cl>0 && p>=0.9*cl)
    {
        w=QString("The conversation uses %1 of %2 tokens of the model's context window. "
                  "When it is full, the server cuts off parts of the input. ").arg(p).arg(cl)
          +remedy;
    }
    lastPromptTokens_=std::max(lastPromptTokens_, p);

    if (!w.isEmpty() && !contextWarned_)
    {
        contextWarned_=true;
        Q_EMIT contextWarning(w);
    }
}


void ISCADAssistantAgent::executeTool(
    const ToolCall& call,
    std::function<void(const ToolResult&)> done )
{
    auto ok = [&](const QString& s) { done(ToolResult{call.id, s, false}); };
    auto err = [&](const QString& s) { done(ToolResult{call.id, s, true}); };

    if (!call.inputValid)
    {
        err("Invalid tool input (no valid JSON object): "+truncated(call.rawInput, 500));
        return;
    }

    const auto& in=call.input;

    if (disabledTools_.contains(call.name)
        || (call.name=="read_reference" && !referenceIndex_) )
    {
        err(QString("The tool \"%1\" is not available.").arg(call.name));
        return;
    }

    if (call.name=="read_script")
    {
        QString text=host_->scriptText();
        int nLines=text.count('\n')+1;
        int from=in.value("from_line").toInt(1);
        int to=in.value("to_line").toInt(nLines);
        if (text.isEmpty())
        {
            ok("(the script is empty)");
            return;
        }
        if (from>nLines)
        {
            err(QString("from_line %1 is beyond the end of the script (%2 lines)").arg(from).arg(nLines));
            return;
        }
        ok(truncated(numberedLines(text, from, to), maxReadChars));
    }
    else if (call.name=="edit_script")
    {
        if (!in.value("old_string").isString() || !in.value("new_string").isString())
        {
            err("old_string and new_string are required");
            return;
        }
        QString oldS=in.value("old_string").toString();
        QString newS=in.value("new_string").toString();
        bool all=in.value("replace_all").toBool(false);
        if (oldS.isEmpty())
        {
            err("old_string must not be empty. Use write_script to fill an empty script.");
            return;
        }
        if (oldS==newS)
        {
            err("old_string and new_string are identical");
            return;
        }
        QString text=host_->scriptText();
        QList<int> pos;
        int n=countOccurrences(text, oldS, &pos);
        if (n==0)
        {
            err("old_string was not found in the script. "
                "Read the script again (read_script); whitespace and line breaks must match exactly.");
            return;
        }
        if (n>1 && !all)
        {
            auto starts=lineStarts(text);
            QStringList lines;
            for (int p: pos)
                lines<<QString::number(lineOf(starts, p));
            err(QString("old_string was found %1 times (lines %2). "
                        "Include more surrounding text to make it unique, or set replace_all.")
                .arg(n).arg(lines.join(", ")));
            return;
        }

        // replace from the end, so that the positions remain valid
        for (int i=pos.size()-1; i>=0; --i)
            host_->replaceText(pos[i], oldS.size(), newS);

        QString newText=host_->scriptText();
        auto starts=lineStarts(newText);
        int first=lineOf(starts, pos.first());
        int last=lineOf(starts, pos.first()+std::max(0, int(newS.size())-1));
        ok(QString("Replaced %1 occurrence(s). The script around the first change now reads:\n%2")
           .arg(n)
           .arg(numberedLines(newText, first-2, std::min(last+2, first+40))));
    }
    else if (call.name=="write_script")
    {
        if (!in.value("content").isString())
        {
            err("content is required");
            return;
        }
        QString c=in.value("content").toString();
        host_->replaceText(0, host_->scriptText().size(), c);
        ok(QString("The script was replaced (%1 lines).").arg(c.count('\n')+1));
    }
    else if (call.name=="rebuild")
    {
        QString text=host_->scriptText();
        int upTo=-1;
        if (in.contains("up_to_line"))
        {
            auto starts=lineStarts(text);
            int l=in.value("up_to_line").toInt();
            if (l>=1 && l<starts.size())
                upTo=starts[l]; // start of the following line
        }

        rebuildRunning_=true;
        Q_EMIT statusMessage("Rebuilding model...");
        auto cb=done;
        host_->rebuild(upTo, [this, cb, text, id=call.id](bool success, const QString& message, int errorPos)
        {
            rebuildRunning_=false;
            if (success)
            {
                QString r="The model was built successfully.";
                if (!message.isEmpty())
                    r+="\n"+message;
                cb(ToolResult{id, r, false});
            }
            else
            {
                QString r="The rebuild failed";
                if (errorPos>=0 && errorPos<=text.size())
                {
                    auto starts=lineStarts(text);
                    int line=lineOf(starts, errorPos);
                    int col=errorPos-starts[line-1]+1;
                    r+=QString(" at line %1, column %2").arg(line).arg(col);
                    r+=":\n"+message+"\n\nScript around the error:\n"
                       +numberedLines(text, line-3, line+3);
                }
                else
                {
                    r+=":\n"+message;
                }
                cb(ToolResult{id, r, true});
            }
        });
    }
    else if (call.name=="list_commands")
    {
        QString filter=in.value("filter").toString();
        auto docs=host_->commandDocumentation();
        QStringList out;
        for (const auto& d: docs)
        {
            if (filter.isEmpty() || d.command.contains(filter, Qt::CaseInsensitive))
                out<<d.command+d.signature;
        }
        out.sort(Qt::CaseInsensitive);
        if (out.isEmpty())
            ok("No matching commands.");
        else
            ok(truncated(out.join('\n'), maxReadChars));
    }
    else if (call.name=="command_help")
    {
        QString name=in.value("name").toString().trimmed();
        auto docs=host_->commandDocumentation();
        const CommandDoc* found=nullptr;
        for (const auto& d: docs)
            if (d.command==name) { found=&d; break; }
        if (!found)
            for (const auto& d: docs)
                if (d.command.compare(name, Qt::CaseInsensitive)==0) { found=&d; break; }

        if (found)
        {
            ok(QString("%1%2\n\n%3").arg(found->command, found->signature, found->documentation));
        }
        else
        {
            QStringList similar;
            for (const auto& d: docs)
                if (d.command.contains(name, Qt::CaseInsensitive)
                    || name.contains(d.command, Qt::CaseInsensitive))
                    similar<<d.command;
            err(QString("Unknown command \"%1\".%2")
                .arg(name)
                .arg(similar.isEmpty() ? QString(" Use list_commands to see all commands.")
                                       : QString(" Similar commands: ")+similar.join(", ")));
        }
    }
    else if (call.name=="read_reference")
    {
        QString name=in.value("section").toString();
        auto r=referenceIndex_->lookup(name);
        if (!r.sections.isEmpty())
        {
            QStringList texts;
            for (const auto* s: r.sections)
                texts<<(r.sections.size()>1 ? QString("[%1]\n").arg(s->path) : QString())+s->text;
            ok(truncated(texts.join("\n\n"), maxReadChars));
        }
        else if (!r.candidates.isEmpty())
        {
            auto c=r.candidates;
            if (c.size()>30)
                c=c.mid(0, 30)<<"...";
            err(QString("\"%1\" matches several sections. Use one of these titles or paths:\n%2")
                .arg(name, c.join('\n')));
        }
        else
        {
            err(QString("There is no section \"%1\". Available sections:\n%2")
                .arg(name, referenceIndex_->tableOfContents()));
        }
    }
    else if (call.name=="list_symbols")
    {
        auto t=host_->symbols();
        done(ToolResult{call.id, truncated(formatSymbols(t), maxReadChars), !t.ok});
    }
    else if (call.name=="inspect_feature")
    {
        QString name=in.value("name").toString().trimmed();
        if (name.isEmpty())
        {
            err("name is required");
            return;
        }
        auto p=host_->featureProperties(name);
        done(ToolResult{call.id, truncated(formatFeatureProperties(name, p), maxReadChars), !p.ok});
    }
    else if (call.name=="view_model")
    {
        QString dir=in.value("direction").toString("iso").trimmed().toLower();
        static const QStringList dirs{"iso", "front", "back", "top", "bottom", "left", "right", "current"};
        if (!dirs.contains(dir))
        {
            err("direction must be one of: "+dirs.join(", "));
            return;
        }
        QByteArray png;
        auto r=host_->captureView(dir, in.value("highlight").toString().trimmed(), png);
        if (!r.ok || png.isEmpty())
        {
            err(r.text.isEmpty() ? QString("No image could be created.") : r.text);
            return;
        }
        Q_EMIT toolImageProduced("image/png", png);
        done(ToolResult{call.id, r.text, false, {ToolImage{"image/png", png}}});
    }
    else if (call.name=="list_library_models")
    {
        QString filter=in.value("filter").toString();
        QStringList names;
        for (const auto& dir: host_->librarySearchPaths())
        {
            QDirIterator it(dir, {"*.iscad"}, QDir::Files);
            while (it.hasNext())
            {
                QString n=QFileInfo(it.next()).completeBaseName();
                if ((filter.isEmpty() || n.contains(filter, Qt::CaseInsensitive))
                    && !names.contains(n))
                    names<<n;
            }
        }
        names.sort(Qt::CaseInsensitive);
        if (names.isEmpty())
            ok("No matching library models.");
        else
            ok(names.join('\n'));
    }
    else if (call.name=="read_library_model")
    {
        QString name=in.value("name").toString().trimmed();
        if (name.endsWith(".iscad"))
            name.chop(6);
        if (name.isEmpty() || name.contains('/') || name.contains('\\') || name.contains(".."))
        {
            err("Invalid model name.");
            return;
        }
        for (const auto& dir: host_->librarySearchPaths())
        {
            QFile f(QDir(dir).filePath(name+".iscad"));
            if (f.open(QIODevice::ReadOnly|QIODevice::Text))
            {
                QString c=QString::fromUtf8(f.readAll());
                ok(truncated(c, maxReadChars));
                return;
            }
        }
        err(QString("Library model \"%1\" not found. Use list_library_models.").arg(name));
    }
    else
    {
        err(QString("Unknown tool \"%1\".").arg(call.name));
    }
}


} // namespace assistant
} // namespace insight
