#include "evaluation.h"

#include <algorithm>
#include <cmath>
#include <climits>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>
#include <QMap>

#include "iscadassistantagent.h"

namespace insight {
namespace assistant {


namespace {


QString num(double v)
{
    return QString::number(v, 'g', 6);
}


struct Tolerance
{
    double rel = 0.005, abs = 0.02;

    explicit Tolerance(const QJsonObject& c)
    {
        rel=c.value("rel_tol").toDouble(rel);
        abs=c.value("abs_tol").toDouble(abs);
    }

    bool matches(double actual, double expected) const
    {
        return std::fabs(actual-expected) <= std::max(abs, rel*std::fabs(expected));
    }
};


bool readVec(const QJsonValue& v, ISCADAssistantHost::Vec3& out)
{
    auto a=v.toArray();
    if (a.size()!=3)
        return false;
    out={a[0].toDouble(), a[1].toDouble(), a[2].toDouble()};
    return true;
}


QString vecStr(const ISCADAssistantHost::Vec3& v)
{
    return QString("[%1, %2, %3]").arg(num(v.x), num(v.y), num(v.z));
}


/**
 * compare the requested properties of a feature
 * @return empty, if all match, otherwise the mismatches
 */
QStringList compareFeature(
    const QJsonObject& c,
    const ISCADAssistantHost::FeatureProperties& p )
{
    QStringList mismatch;
    Tolerance tol(c);

    if (!p.ok)
        return {p.error};

    auto scalar = [&](const char* key, const std::optional<double>& actual)
    {
        if (!c.contains(key)) return;
        double e=c.value(key).toDouble();
        if (!actual)
            mismatch<<QString("%1 not available").arg(key);
        else if (!tol.matches(*actual, e))
            mismatch<<QString("%1 is %2, expected %3").arg(key, num(*actual), num(e));
    };
    scalar("volume", p.volume);
    scalar("area", p.surfaceArea);

    std::optional<ISCADAssistantHost::Vec3> size;
    if (p.bbMin && p.bbMax)
        size=ISCADAssistantHost::Vec3{p.bbMax->x-p.bbMin->x, p.bbMax->y-p.bbMin->y, p.bbMax->z-p.bbMin->z};

    auto vector = [&](const char* key, const std::optional<ISCADAssistantHost::Vec3>& actual)
    {
        if (!c.contains(key)) return;
        ISCADAssistantHost::Vec3 e;
        if (!readVec(c.value(key), e))
        {
            mismatch<<QString("invalid %1 in check").arg(key);
            return;
        }
        // the bounding box is computed from a tessellation and
        // underestimates curved shapes: 1 % of the largest dimension
        Tolerance vt=tol;
        if (QString(key).startsWith("bbox"))
        {
            double m=std::max({std::fabs(e.x), std::fabs(e.y), std::fabs(e.z)});
            if (QString(key)!="bbox_size" && size)
                m=std::max({m, size->x, size->y, size->z});
            vt.abs=std::max(vt.abs, c.value("bbox_rel_tol").toDouble(0.01)*m);
        }
        if (!actual)
            mismatch<<QString("%1 not available").arg(key);
        else if (!(vt.matches(actual->x, e.x) && vt.matches(actual->y, e.y) && vt.matches(actual->z, e.z)))
            mismatch<<QString("%1 is %2, expected %3").arg(key, vecStr(*actual), vecStr(e));
    };
    vector("bbox_size", size);
    vector("bbox_min", p.bbMin);
    vector("bbox_max", p.bbMax);
    vector("cog", p.cog);

    auto count = [&](const char* key, int actual)
    {
        if (!c.contains(key)) return;
        int e=c.value(key).toInt();
        if (actual!=e)
            mismatch<<QString("%1 is %2, expected %3").arg(key).arg(actual).arg(e);
    };
    count("n_solids", p.nSolids);
    count("n_faces", p.nFaces);

    return mismatch;
}


/**
 * all numbers in a text (also with thousands separators)
 */
QList<double> numbersIn(const QString& text)
{
    QList<double> r;
    QRegularExpression re("[-+]?(?:\\d{1,3}(?:[,' ]\\d{3})+|\\d+)(?:\\.\\d+)?(?:[eE][-+]?\\d+)?");
    auto it=re.globalMatch(text);
    while (it.hasNext())
    {
        QString s=it.next().captured(0);
        bool ok=false;
        double v=s.toDouble(&ok);
        if (!ok)
        {
            QString t=s;
            t.remove(',').remove('\'').remove(' ');
            v=t.toDouble(&ok);
        }
        if (ok)
            r.append(v);
    }
    return r;
}


QJsonObject usageJson(const Usage& u)
{
    return QJsonObject{
        {"input_tokens", double(u.inputTokens)},
        {"output_tokens", double(u.outputTokens)},
        {"cache_read_tokens", double(u.cacheReadTokens)},
        {"cache_write_tokens", double(u.cacheWriteTokens)}
    };
}


QJsonArray transcriptJson(const QList<ChatMessage>& h)
{
    QJsonArray a;
    for (const auto& m: h)
    {
        QJsonObject o{ {"role", m.role==ChatMessage::User ? "user" : "assistant"} };
        if (!m.text.isEmpty())
            o["text"]=m.text;
        if (!m.toolCalls.isEmpty())
        {
            QJsonArray tc;
            for (const auto& c: m.toolCalls)
                tc.append(QJsonObject{ {"id", c.id}, {"name", c.name}, {"input", c.input} });
            o["tool_calls"]=tc;
        }
        if (!m.toolResults.isEmpty())
        {
            QJsonArray tr;
            for (const auto& r: m.toolResults)
            {
                QJsonObject ro{
                    {"id", r.toolCallId},
                    {"content", r.content.left(4000)},
                    {"is_error", r.isError}
                };
                if (!r.images.isEmpty())
                    ro["images"]=r.images.size();
                tr.append(ro);
            }
            o["tool_results"]=tr;
        }
        a.append(o);
    }
    return a;
}


/**
 * rebuild and wait for the result
 */
void rebuildAndWait(ISCADAssistantHost* host, int timeoutSeconds, bool& ok, QString& msg)
{
    ok=false;
    msg.clear();
    bool built=false;
    QEventLoop loop;
    host->rebuild(-1, [&](bool o, const QString& m, int)
    {
        ok=o;
        msg=m;
        built=true;
        loop.quit();
    });
    if (!built)
    {
        QTimer t;
        t.setSingleShot(true);
        QObject::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
        t.start(timeoutSeconds*1000);
        loop.exec();
        if (!built)
        {
            host->cancelRebuild();
            msg="rebuild timed out";
        }
    }
}


} // anonymous namespace




// ================================================================
// EvalTask
// ================================================================


EvalTask EvalTask::fromJson(const QJsonObject& o, const QString& baseDir, QString* error)
{
    EvalTask t;
    t.id=o.value("id").toString();
    t.category=o.value("category").toString();
    t.description=o.value("description").toString();
    t.prompt=o.value("prompt").toString();
    t.script=o.value("script").toString();
    t.checks=o.value("checks").toArray();
    t.maxSteps=o.value("max_steps").toInt(0);
    t.startBuilds=o.value("start_builds").toBool(true);
    t.solution=o.value("solution").toString();
    t.solutionAnswer=o.value("solution_answer").toString();

    if (o.contains("script_file"))
    {
        QString fn=QDir(baseDir).filePath(o.value("script_file").toString());
        QFile f(fn);
        if (!f.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            if (error) *error=QString("task %1: cannot read %2").arg(t.id, fn);
            return EvalTask();
        }
        t.script=QString::fromUtf8(f.readAll());
        t.fileName=QFileInfo(fn).fileName();
    }

    if (t.id.isEmpty() || t.prompt.isEmpty())
    {
        if (error) *error="task without id or prompt";
        return EvalTask();
    }
    if (t.checks.isEmpty())
    {
        if (error) *error=QString("task %1 has no checks").arg(t.id);
        return EvalTask();
    }
    return t;
}


QList<EvalTask> EvalTask::loadDirectory(const QString& dir, QString* error)
{
    QList<EvalTask> tasks;
    QDir d(dir);
    auto files=d.entryList({"*.json"}, QDir::Files, QDir::Name);
    if (files.isEmpty())
    {
        if (error) *error=QString("no task files (*.json) in %1").arg(dir);
        return tasks;
    }
    for (const auto& fn: files)
    {
        QFile f(d.filePath(fn));
        if (!f.open(QIODevice::ReadOnly))
        {
            if (error) *error=QString("cannot read %1").arg(fn);
            return QList<EvalTask>();
        }
        QJsonParseError pe;
        auto doc=QJsonDocument::fromJson(f.readAll(), &pe);
        if (pe.error!=QJsonParseError::NoError)
        {
            if (error) *error=QString("%1: %2 at offset %3").arg(fn, pe.errorString()).arg(pe.offset);
            return QList<EvalTask>();
        }

        // a file contains one task or an array of tasks
        QJsonArray list = doc.isArray() ? doc.array() : QJsonArray{doc.object()};
        for (const auto& v: list)
        {
            QString err;
            auto t=fromJson(v.toObject(), d.absolutePath(), &err);
            if (!err.isEmpty())
            {
                if (error) *error=QString("%1: %2").arg(fn, err);
                return QList<EvalTask>();
            }
            for (const auto& o: tasks)
            {
                if (o.id==t.id)
                {
                    if (error) *error=QString("%1: duplicate task id %2").arg(fn, t.id);
                    return QList<EvalTask>();
                }
            }
            tasks.append(t);
        }
    }
    return tasks;
}




// ================================================================
// checks
// ================================================================


QList<CheckResult> evaluateChecks(const QJsonArray& checks, const EvalContext& ctx)
{
    QList<CheckResult> results;
    const QString script=ctx.host->scriptText();

    for (const auto& cv: checks)
    {
        auto c=cv.toObject();
        QString type=c.value("type").toString();
        CheckResult r;
        r.description=c.value("description").toString(type);

        if (type=="builds")
        {
            r.passed=ctx.finalBuildOk;
            if (!r.passed)
                r.message="final script does not build: "+ctx.finalBuildMessage.left(300);
        }
        else if (type=="script_contains" || type=="script_not_contains")
        {
            QString t=c.value("text").toString();
            bool has=script.contains(t);
            r.passed = (type=="script_contains") ? has : !has;
            if (!r.passed)
                r.message=QString("script %1 \"%2\"").arg(has ? "contains" : "does not contain", t);
        }
        else if (type=="script_matches" || type=="script_not_matches")
        {
            QRegularExpression re(c.value("regex").toString(), QRegularExpression::MultilineOption);
            if (!re.isValid())
            {
                r.message="invalid regex: "+re.errorString();
            }
            else
            {
                bool m=re.match(script).hasMatch();
                r.passed = (type=="script_matches") ? m : !m;
                if (!r.passed)
                    r.message=QString("script %1 /%2/").arg(m ? "matches" : "does not match", re.pattern());
            }
        }
        else if (type=="script_unchanged")
        {
            r.passed = script==ctx.initialScript;
            if (!r.passed)
                r.message="the script was modified";
        }
        else if (type=="scalar")
        {
            QString name=c.value("name").toString();
            auto t=ctx.host->symbols();
            if (!t.ok)
            {
                r.message=t.error;
            }
            else
            {
                r.message=QString("scalar %1 not found").arg(name);
                for (const auto& s: t.scalars)
                {
                    if (s.first==name)
                    {
                        double e=c.value("value").toDouble();
                        if (!s.second)
                            r.message=QString("scalar %1 not evaluated").arg(name);
                        else
                        {
                            r.passed=Tolerance(c).matches(*s.second, e);
                            r.message = r.passed ? QString() :
                                QString("%1 is %2, expected %3").arg(name, num(*s.second), num(e));
                        }
                        break;
                    }
                }
            }
        }
        else if (type=="feature")
        {
            QString name=c.value("feature").toString();
            if (name.isEmpty() || name=="*")
            {
                auto t=ctx.host->symbols();
                if (!t.ok)
                {
                    r.message=t.error;
                }
                else
                {
                    bool onlyComponents=c.value("component").toBool(false);
                    QStringList tried;
                    for (const auto& f: t.features)
                    {
                        if (!f.built || (onlyComponents && !f.isComponent))
                            continue;
                        auto mm=compareFeature(c, ctx.host->featureProperties(f.name));
                        if (mm.isEmpty())
                        {
                            r.passed=true;
                            r.message=QString("matched by %1").arg(f.name);
                            break;
                        }
                        tried<<QString("%1: %2").arg(f.name, mm.join("; "));
                    }
                    if (!r.passed)
                        r.message = tried.isEmpty() ? QString("no built feature") :
                                                      "no matching feature ("+tried.join(" | ").left(600)+")";
                }
            }
            else
            {
                auto mm=compareFeature(c, ctx.host->featureProperties(name));
                r.passed=mm.isEmpty();
                if (!r.passed)
                    r.message=QString("%1: %2").arg(name, mm.join("; "));
            }
        }
        else if (type=="feature_count")
        {
            auto t=ctx.host->symbols();
            if (!t.ok)
            {
                r.message=t.error;
            }
            else
            {
                bool onlyComponents=c.value("component").toBool(false);
                int n=0;
                for (const auto& f: t.features)
                    if (f.built && (!onlyComponents || f.isComponent))
                        ++n;
                int mn=c.value("min").toInt(0), mx=c.value("max").toInt(INT_MAX);
                r.passed = n>=mn && n<=mx;
                if (!r.passed)
                    r.message=QString("%1 features").arg(n);
            }
        }
        else if (type=="answer_number")
        {
            double e=c.value("value").toDouble();
            Tolerance tol(c);
            for (double v: numbersIn(ctx.finalAnswer))
                if (tol.matches(v, e))
                    r.passed=true;
            if (!r.passed)
                r.message=QString("the answer does not contain %1").arg(num(e));
        }
        else if (type=="answer_contains")
        {
            QString t=c.value("text").toString();
            r.passed=ctx.finalAnswer.contains(t, Qt::CaseInsensitive);
            if (!r.passed)
                r.message=QString("the answer does not contain \"%1\"").arg(t);
        }
        else
        {
            r.message=QString("unknown check type \"%1\"").arg(type);
        }

        results.append(r);
    }
    return results;
}




// ================================================================
// TaskResult
// ================================================================


QJsonObject TaskResult::toJson() const
{
    QJsonArray ca;
    for (const auto& c: checks)
        ca.append(QJsonObject{ {"description", c.description}, {"passed", c.passed}, {"message", c.message} });

    QJsonObject o{
        {"task", taskId},
        {"category", category},
        {"run", run},
        {"passed", passed},
        {"checks", ca},
        {"finish_reason", finishReason},
        {"errors", QJsonArray::fromStringList(errors)},
        {"model_responses", modelResponses},
        {"tool_calls", toolCalls},
        {"failed_tool_calls", failedToolCalls},
        {"usage", usageJson(usage)},
        {"seconds", seconds},
        {"final_script", finalScript},
        {"final_answer", finalAnswer},
        {"transcript", transcript}
    };
    if (cost>=0)
        o["cost_usd"]=cost;
    return o;
}




// ================================================================
// EvalRunner
// ================================================================


EvalRunner::EvalRunner(HostFactory hf, BackendFactory bf, const Config& cfg, QObject* parent)
    : QObject(parent),
      hostFactory_(hf),
      backendFactory_(bf),
      cfg_(cfg)
{}


TaskResult EvalRunner::runTask(const EvalTask& task, int run)
{
    TaskResult res;
    res.taskId=task.id;
    res.category=task.category;
    res.run=run;

    Q_EMIT taskStarted(task.id, run);

    std::unique_ptr<ISCADAssistantHost> host(hostFactory_(task));

    ISCADAssistantAgent agent(host.get());
    agent.setSystemPrompt(cfg_.systemPrompt);
    agent.setDisabledTools(cfg_.disabledTools);
    agent.setMaxSteps(task.maxSteps>0 ? task.maxSteps : cfg_.maxSteps);
    agent.setBackend(backendFactory_());

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    bool timedOut=false;

    connect(&agent, &ISCADAssistantAgent::toolCallFinished, this,
            [&res, this](const QString& name, const QString& summary, bool isError)
            {
                ++res.toolCalls;
                if (isError) ++res.failedToolCalls;
                Q_EMIT progress(QString("  %1%2 %3").arg(name, isError ? " (error)" : "", summary.left(100)));
            });
    connect(&agent, &ISCADAssistantAgent::errorOccurred, this,
            [&res](const QString& msg) { res.errors<<msg; });
    connect(&agent, &ISCADAssistantAgent::statusMessage, this,
            [this](const QString& msg) { Q_EMIT progress("  "+msg); });
    connect(&agent, &ISCADAssistantAgent::requestFinished, &loop,
            [&res, &loop](const QString& reason) { res.finishReason=reason; loop.quit(); });
    connect(&timeout, &QTimer::timeout, &loop, [&]()
            {
                timedOut=true;
                agent.stop();
                loop.quit();
            });

    QElapsedTimer clock;
    clock.start();

    timeout.start(cfg_.timeoutSeconds*1000);
    agent.sendUserMessage(task.prompt);
    if (agent.isRunning())
        loop.exec();
    timeout.stop();
    if (timedOut)
        res.finishReason="timeout";

    res.seconds=double(clock.elapsed())/1000.;

    // ===== final state
    EvalContext ctx;
    ctx.host=host.get();
    ctx.initialScript=task.script;

    rebuildAndWait(host.get(), cfg_.timeoutSeconds, ctx.finalBuildOk, ctx.finalBuildMessage);

    for (int i=agent.history().size()-1; i>=0; --i)
    {
        const auto& m=agent.history()[i];
        if (m.role==ChatMessage::Assistant && !m.text.trimmed().isEmpty())
        {
            ctx.finalAnswer=m.text;
            break;
        }
    }

    res.checks=evaluateChecks(task.checks, ctx);
    res.passed=!res.checks.isEmpty();
    for (const auto& c: res.checks)
        res.passed = res.passed && c.passed;

    for (const auto& m: agent.history())
        if (m.role==ChatMessage::Assistant)
            ++res.modelResponses;
    res.usage=agent.sessionUsage();
    if (agent.backend())
    {
        auto p=agent.backend()->pricing();
        if (p.known)
            res.cost=p.cost(res.usage);
    }
    res.finalScript=host->scriptText();
    res.finalAnswer=ctx.finalAnswer;
    res.transcript=transcriptJson(agent.history());

    Q_EMIT taskFinished(res);
    return res;
}


QList<TaskResult> EvalRunner::run(const QList<EvalTask>& tasks)
{
    QList<TaskResult> results;
    for (int r=0; r<std::max(1, cfg_.repeats); ++r)
        for (const auto& t: tasks)
            results.append(runTask(t, r));
    return results;
}


QStringList EvalRunner::validateTask(const EvalTask& task)
{
    QStringList problems;

    if (!task.script.isEmpty())
    {
        std::unique_ptr<ISCADAssistantHost> host(hostFactory_(task));
        bool ok; QString msg;
        rebuildAndWait(host.get(), cfg_.timeoutSeconds, ok, msg);
        if (ok && !task.startBuilds)
            problems<<"the initial script builds, but start_builds is false";
        else if (!ok && task.startBuilds)
            problems<<"the initial script does not build: "+msg.left(300);
    }

    if (task.solution.isEmpty())
    {
        problems<<"no reference solution";
        return problems;
    }

    EvalTask st=task;
    st.script=task.solution;
    std::unique_ptr<ISCADAssistantHost> host(hostFactory_(st));

    EvalContext ctx;
    ctx.host=host.get();
    // the solution of a task, which must not change the script, is the script itself
    ctx.initialScript=task.script;
    ctx.finalAnswer=task.solutionAnswer;
    rebuildAndWait(host.get(), cfg_.timeoutSeconds, ctx.finalBuildOk, ctx.finalBuildMessage);

    for (const auto& c: evaluateChecks(task.checks, ctx))
        if (!c.passed)
            problems<<QString("solution fails check %1: %2").arg(c.description, c.message);

    return problems;
}


QJsonObject EvalRunner::report(const QList<TaskResult>& results, const QJsonObject& meta)
{
    QJsonArray ra;
    int passed=0;
    double cost=0, seconds=0;
    bool costKnown=true;
    Usage total;
    for (const auto& r: results)
    {
        ra.append(r.toJson());
        if (r.passed) ++passed;
        if (r.cost>=0) cost+=r.cost; else costKnown=false;
        seconds+=r.seconds;
        total+=r.usage;
    }

    QJsonObject summary{
        {"runs", results.size()},
        {"passed", passed},
        {"pass_rate", results.isEmpty() ? 0. : double(passed)/results.size()},
        {"seconds", seconds},
        {"usage", usageJson(total)}
    };
    if (costKnown && !results.isEmpty())
        summary["cost_usd"]=cost;

    return QJsonObject{ {"meta", meta}, {"summary", summary}, {"results", ra} };
}


QString EvalRunner::markdownReport(const QList<TaskResult>& results, const QJsonObject& meta)
{
    auto rep=report(results, meta);
    auto s=rep.value("summary").toObject();

    QString md="# ISCAD assistant evaluation\n\n";
    for (auto it=meta.begin(); it!=meta.end(); ++it)
        md+=QString("- **%1:** %2\n").arg(it.key(), it.value().toVariant().toString());
    md+=QString("\n**Passed %1 of %2 (%3 %)**")
            .arg(s.value("passed").toInt()).arg(s.value("runs").toInt())
            .arg(100.*s.value("pass_rate").toDouble(), 0, 'f', 0);
    md+=QString(", %1 s").arg(s.value("seconds").toDouble(), 0, 'f', 0);
    if (s.contains("cost_usd"))
        md+=QString(", $%1").arg(s.value("cost_usd").toDouble(), 0, 'f', 2);
    md+="\n\n";

    // pass rate per category
    QMap<QString, QPair<int,int> > cat;
    for (const auto& r: results)
    {
        auto& c=cat[r.category.isEmpty() ? QString("-") : r.category];
        ++c.second;
        if (r.passed) ++c.first;
    }
    md+="| Category | Passed |\n|---|---|\n";
    for (auto it=cat.begin(); it!=cat.end(); ++it)
        md+=QString("| %1 | %2/%3 |\n").arg(it.key()).arg(it.value().first).arg(it.value().second);

    md+="\n| Task | Run | Result | Steps | Tool calls (failed) | Tokens in/cached/out | Cost | Time | Failed checks |\n"
        "|---|---|---|---|---|---|---|---|---|\n";
    for (const auto& r: results)
    {
        QStringList failed;
        for (const auto& c: r.checks)
            if (!c.passed)
                failed<<QString("%1: %2").arg(c.description, c.message);
        if (r.finishReason!="end_turn" && !r.finishReason.isEmpty())
            failed.prepend("ended: "+r.finishReason);
        QString f=failed.join("; ").replace('|', '/').replace('\n', ' ').left(300);
        md+=QString("| %1 | %2 | %3 | %4 | %5 (%6) | %7/%8/%9 | %10 | %11 s | %12 |\n")
                .arg(r.taskId).arg(r.run+1)
                .arg(r.passed ? "pass" : "**FAIL**")
                .arg(r.modelResponses).arg(r.toolCalls).arg(r.failedToolCalls)
                .arg(r.usage.inputTokens+r.usage.cacheWriteTokens).arg(r.usage.cacheReadTokens).arg(r.usage.outputTokens)
                .arg(r.cost>=0 ? QString("$%1").arg(r.cost, 0, 'f', 3) : QString("-"))
                .arg(r.seconds, 0, 'f', 0)
                .arg(f);
    }
    return md;
}


} // namespace assistant
} // namespace insight
