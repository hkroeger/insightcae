/*
 * iscad-assistant-eval: automated evaluation of the ISCAD chat assistant.
 *
 * Runs the modeling tasks (share/insight/iscad-assistant-eval/ *.json) with a
 * language model, builds the resulting scripts and checks the geometry.
 * Writes a JSON report (with transcripts) and a markdown summary.
 */

#include <iostream>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>

#include "base/sharedpathlist.h"

#include "assistant/anthropicbackend.h"
#include "assistant/openaicompatbackend.h"
#include "assistant/iscadassistantagent.h"
#include "assistant/evaluation.h"

#include "iscadheadlesshost.h"
#include "iscadmodelinspection.h"


using namespace insight::assistant;


namespace {

QString defaultTaskDirectory()
{
    QString env=qEnvironmentVariable("ISCAD_ASSISTANT_EVAL_TASKS");
    if (!env.isEmpty())
        return env;
    for (const auto& p: insight::SharedPathList::global())
    {
        auto d=p/"iscad-assistant-eval";
        if (boost::filesystem::is_directory(d))
            return QString::fromStdString(d.string());
    }
    return QString();
}

}


int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("iscad-assistant-eval");

    QCommandLineParser p;
    p.setApplicationDescription(
        "Runs modeling tasks with the ISCAD chat assistant and checks the results." );
    p.addHelpOption();

    QCommandLineOption
        oTasks("tasks", "directory with the task files (*.json)", "dir", defaultTaskDirectory()),
        oList("list", "list the tasks and exit"),
        oValidate("validate", "check the task definitions without a model: initial scripts and "
                              "reference solutions are built and the checks are applied to the solutions"),
        oFilter("filter", "run only tasks, whose id or category matches this regular expression", "regex"),
        oProvider("provider", "anthropic or openai (any OpenAI compatible server)", "name", "anthropic"),
        oModel("model", "model id (default for anthropic: claude-opus-5-5)", "id"),
        oBaseUrl("base-url", "API base URL (openai: e.g. http://localhost:11434/v1)", "url"),
        oApiKey("api-key", "API key (default: ANTHROPIC_API_KEY or OPENAI_API_KEY)", "key"),
        oEffort("effort", "anthropic: low, medium, high, xhigh, max", "level", "high"),
        oMaxTokens("max-tokens", "openai: maximum output tokens per response (0: server default)", "n", "0"),
        oPriceIn("price-input", "openai: USD per million input tokens", "usd", "0"),
        oPriceOut("price-output", "openai: USD per million output tokens", "usd", "0"),
        oPriceCached("price-cached", "openai: USD per million cached input tokens", "usd", "0"),
        oReference("reference", "language reference in the system prompt: full, basic or none", "level", "full"),
        oTools("tools", "tool set: full or basic (for small models)", "set", "full"),
        oRepeat("repeat", "run every task n times", "n", "1"),
        oMaxSteps("max-steps", "maximum model responses per task", "n", "40"),
        oTimeout("timeout", "maximum time per task in seconds", "s", "900"),
        oOut("out", "output file prefix (writes <prefix>.json and <prefix>.md)", "prefix");

    p.addOptions({oTasks, oList, oValidate, oFilter, oProvider, oModel, oBaseUrl, oApiKey, oEffort, oMaxTokens,
                  oPriceIn, oPriceOut, oPriceCached, oReference, oTools, oRepeat, oMaxSteps,
                  oTimeout, oOut});
    p.process(app);

    // ===== tasks
    QString taskDir=p.value(oTasks);
    if (taskDir.isEmpty())
    {
        std::cerr<<"No task directory found. Use --tasks."<<std::endl;
        return 2;
    }
    QString error;
    auto tasks=EvalTask::loadDirectory(taskDir, &error);
    if (!error.isEmpty())
    {
        std::cerr<<error.toStdString()<<std::endl;
        return 2;
    }
    if (p.isSet(oFilter))
    {
        QRegularExpression re(p.value(oFilter));
        QList<EvalTask> sel;
        for (const auto& t: tasks)
            if (re.match(t.id).hasMatch() || re.match(t.category).hasMatch())
                sel.append(t);
        tasks=sel;
    }
    if (p.isSet(oList))
    {
        for (const auto& t: tasks)
            std::cout<<t.id.toStdString()<<"\t"<<t.category.toStdString()<<"\t"
                     <<t.description.toStdString()<<std::endl;
        return 0;
    }
    if (tasks.isEmpty())
    {
        std::cerr<<"No tasks selected."<<std::endl;
        return 2;
    }

    if (p.isSet(oValidate))
    {
        EvalRunner::Config vc;
        vc.timeoutSeconds=p.value(oTimeout).toInt();
        EvalRunner validator(
            [](const EvalTask& t) { return new ISCADHeadlessHost(t.script, t.fileName); },
            EvalRunner::BackendFactory(), vc );
        int nFailed=0;
        for (const auto& t: tasks)
        {
            auto problems=validator.validateTask(t);
            std::cout<<(problems.isEmpty() ? "ok    " : "FAILED")<<" "<<t.id.toStdString()<<std::endl;
            for (const auto& pr: problems)
                std::cout<<"       "<<pr.toStdString()<<std::endl;
            if (!problems.isEmpty())
                ++nFailed;
        }
        std::cout<<nFailed<<" of "<<tasks.size()<<" task definitions have problems."<<std::endl;
        return nFailed>0 ? 1 : 0;
    }

    // ===== model
    QString provider=p.value(oProvider).toLower();
    EvalRunner::BackendFactory backendFactory;
    QString modelName;
    if (provider=="anthropic")
    {
        AnthropicConfig c;
        c.apiKey = p.isSet(oApiKey) ? p.value(oApiKey) : qEnvironmentVariable("ANTHROPIC_API_KEY");
        if (p.isSet(oModel)) c.model=p.value(oModel);
        if (p.isSet(oBaseUrl)) c.baseUrl=QUrl(p.value(oBaseUrl));
        c.effort=p.value(oEffort);
        if (c.apiKey.isEmpty())
        {
            std::cerr<<"No API key (set ANTHROPIC_API_KEY or use --api-key)."<<std::endl;
            return 2;
        }
        modelName=c.model;
        backendFactory=[c]() { return new AnthropicBackend(c); };
    }
    else if (provider=="openai")
    {
        OpenAICompatConfig c;
        c.apiKey = p.isSet(oApiKey) ? p.value(oApiKey) : qEnvironmentVariable("OPENAI_API_KEY");
        if (p.isSet(oBaseUrl)) c.baseUrl=QUrl(p.value(oBaseUrl));
        c.model=p.value(oModel);
        c.maxTokens=p.value(oMaxTokens).toInt();
        c.priceInput=p.value(oPriceIn).toDouble();
        c.priceOutput=p.value(oPriceOut).toDouble();
        c.priceCachedInput=p.value(oPriceCached).toDouble();
        if (c.model.isEmpty())
        {
            std::cerr<<"--model is required for provider openai."<<std::endl;
            return 2;
        }
        modelName=c.model;
        backendFactory=[c]() { return new OpenAICompatBackend(c); };
    }
    else
    {
        std::cerr<<"Unknown provider "<<provider.toStdString()<<std::endl;
        return 2;
    }

    // ===== prompt and tools
    QString ref=p.value(oReference).toLower();
    auto level =
        ref=="none" ? ISCADModelInspection::NoReference :
        ref=="basic" ? ISCADModelInspection::BasicReference :
                       ISCADModelInspection::FullReference;

    EvalRunner::Config cfg;
    cfg.systemPrompt=ISCADAssistantAgent::defaultSystemPrompt(
        ISCADModelInspection::languageReference(level) );
    if (p.value(oTools).toLower()=="basic")
        cfg.disabledTools=ISCADAssistantAgent::toolsNotIn(ISCADAssistantAgent::basicToolSet());
    if (!cfg.disabledTools.contains("view_model"))
        cfg.disabledTools<<"view_model"; // no 3D view without GUI
    cfg.maxSteps=p.value(oMaxSteps).toInt();
    cfg.timeoutSeconds=p.value(oTimeout).toInt();
    cfg.repeats=std::max(1, p.value(oRepeat).toInt());

    EvalRunner runner(
        [](const EvalTask& t) { return new ISCADHeadlessHost(t.script, t.fileName); },
        backendFactory,
        cfg );

    QObject::connect(&runner, &EvalRunner::taskStarted, [](const QString& id, int run)
    {
        std::cout<<"=== "<<id.toStdString()<<" (run "<<run+1<<")"<<std::endl;
    });
    QObject::connect(&runner, &EvalRunner::progress, [](const QString& msg)
    {
        std::cout<<msg.toStdString()<<std::endl;
    });
    QObject::connect(&runner, &EvalRunner::taskFinished, [](const TaskResult& r)
    {
        std::cout<<(r.passed ? "PASS" : "FAIL")<<" "<<r.taskId.toStdString()
                 <<" ("<<r.modelResponses<<" responses, "<<r.toolCalls<<" tool calls, "
                 <<int(r.seconds)<<" s"
                 <<(r.cost>=0 ? QString(", $%1").arg(r.cost, 0, 'f', 3).toStdString() : std::string())
                 <<")"<<std::endl;
        for (const auto& c: r.checks)
            if (!c.passed)
                std::cout<<"  failed: "<<c.description.toStdString()<<": "<<c.message.toStdString()<<std::endl;
    });

    auto results=runner.run(tasks);

    // ===== reports
    QJsonObject meta{
        {"date", QDateTime::currentDateTime().toString(Qt::ISODate)},
        {"provider", provider},
        {"model", modelName},
        {"reference", ref},
        {"tools", p.value(oTools)},
        {"tasks", taskDir},
        {"repeats", cfg.repeats}
    };
    if (provider=="anthropic")
        meta["effort"]=p.value(oEffort);

    QString prefix = p.isSet(oOut) ? p.value(oOut) :
        QString("iscad-assistant-eval-%1-%2")
            .arg(QString(modelName).replace(QRegularExpression("[^A-Za-z0-9._-]"), "_"))
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));

    {
        QFile f(prefix+".json");
        if (f.open(QIODevice::WriteOnly))
            f.write(QJsonDocument(EvalRunner::report(results, meta)).toJson());
    }
    QString md=EvalRunner::markdownReport(results, meta);
    {
        QFile f(prefix+".md");
        if (f.open(QIODevice::WriteOnly|QIODevice::Text))
            f.write(md.toUtf8());
    }

    std::cout<<std::endl<<md.section("\n\n", 0, 1).toStdString()<<std::endl
             <<"Reports: "<<prefix.toStdString()<<".json, "<<prefix.toStdString()<<".md"<<std::endl;

    return 0;
}
