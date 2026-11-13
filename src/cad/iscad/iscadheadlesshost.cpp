#include "iscadheadlesshost.h"

#include <sstream>
#include <future>
#include <thread>
#include <chrono>
#include <functional>

#include "parser.h"
#include "cadfeature.h"
#include "cadexception.h"
#include "cadmodel.h"
#include "datum.h"
#include "cadpostprocaction.h"

#include "iscadmodelinspection.h"
#include "scriptpositionmap.h"


using namespace insight::assistant;


std::atomic<int> ISCADHeadlessHost::nAbandoned_{0};


namespace {


/**
 * runs f in a worker thread and waits at most timeLimit seconds (<=0: no limit).
 * Returns false, if the time limit was exceeded: the worker is then abandoned.
 * f must only use data, which it owns (captured by value).
 */
template<class R>
bool runLimited(int timeLimit, std::function<R()> f, R& result)
{
    auto task=std::make_shared<std::packaged_task<R()> >(std::move(f));
    auto fut=task->get_future();
    std::thread([task]() { (*task)(); }).detach();
    if (timeLimit>0
        && fut.wait_for(std::chrono::seconds(timeLimit))!=std::future_status::ready)
        return false;
    result=fut.get();
    return true;
}


struct ModelBuild
{
    ISCADHeadlessHost::BuildResult result;
    std::shared_ptr<insight::cad::Model> model; // null, if parsing failed
};


ModelBuild buildModel(const std::string& script)
{
    ScriptPositionMap pm(script);

    auto model=std::make_shared<insight::cad::Model>();
    insight::cad::parser::SyntaxElementDirectoryPtr sd;

    // ===== parse
    try
    {
        std::istringstream is(script);
        int failloc=-1;
        if (!insight::cad::parseISCADModelStream(is, model.get(), &failloc, &sd))
        {
            return {{false, "Parsing of model script failed", failloc>=0 ? pm.toChar(failloc) : -1}, nullptr};
        }
    }
    catch (const insight::cad::parser::iscadParserException& e)
    {
        return {{false, QString::fromStdString(e.summary()), pm.toChar(e.from_pos())}, nullptr};
    }
    catch (const insight::Exception& e)
    {
        return {{false, QString::fromStdString(e), -1}, nullptr};
    }
    catch (const std::exception& e)
    {
        return {{false, QString::fromStdString(e.what()), -1}, nullptr};
    }

    // ===== build everything (as the editor does, without writing postproc output)
    // on failure, the partially built model is kept: inspection of the built features is possible
    try
    {
        for (const auto& v: model->scalars())
            v.second->value();
        for (const auto& v: model->points())
            v.second->value();
        for (const auto& v: model->directions())
            v.second->value();
        for (const auto& v: model->modelsteps())
            v.second->checkForBuildDuringAccess();
        for (const auto& v: model->datums())
            v.second->checkForBuildDuringAccess();
        for (const auto& v: model->postprocActions())
            v.second->checkForBuildDuringAccess();
    }
    catch (const insight::CADException& e)
    {
        int pos=-1;
        if (sd)
        {
            auto loc=sd->findLocation(
                std::const_pointer_cast<insight::cad::Feature>(e.description()->geometryInError_) );
            if (loc.second.first>=0)
                pos=pm.toChar(loc.second.first);
        }
        return {{false, QString::fromStdString(e), pos}, model};
    }
    catch (const insight::Exception& e)
    {
        return {{false, QString::fromStdString(e), -1}, model};
    }
    catch (const std::exception& e)
    {
        return {{false, QString::fromStdString(e.what()), -1}, model};
    }

    return {{true, QString(), -1}, model};
}


} // anonymous namespace


ISCADHeadlessHost::ISCADHeadlessHost(const QString& script, const QString& fileName, int timeLimit)
    : script_(script),
      fileName_(fileName),
      timeLimit_(timeLimit)
{}


int ISCADHeadlessHost::nAbandonedWorkers()
{
    return nAbandoned_;
}


void ISCADHeadlessHost::dropModelAfterTimeout(const QString& what)
{
    ++nAbandoned_;
    model_.reset();
    modelCurrent_=false;
    timeoutMessage_=QString(
        "%1 took longer than %2 s and was aborted. The model was discarded. "
        "The script probably contains degenerate geometry (e.g. a shape without volume, "
        "zero thickness or self-intersections). Change the script and call rebuild." )
            .arg(what).arg(timeLimit_);
}


ISCADHeadlessHost::~ISCADHeadlessHost()
{}


QString ISCADHeadlessHost::scriptText() const
{
    return script_;
}


QString ISCADHeadlessHost::scriptFileName() const
{
    return fileName_;
}


void ISCADHeadlessHost::beginAssistantTurn()
{}


void ISCADHeadlessHost::endAssistantTurn()
{}


void ISCADHeadlessHost::replaceText(int pos, int length, const QString& text)
{
    script_.replace(pos, length, text);
    modelCurrent_=false;
}


ISCADHeadlessHost::BuildResult ISCADHeadlessHost::build(int upToPos)
{
    QString text = (upToPos>=0 && upToPos<script_.size()) ? script_.left(upToPos) : script_;
    std::string script=text.toStdString();

    ModelBuild b;
    if (!runLimited<ModelBuild>(timeLimit_, [script]() { return buildModel(script); }, b))
    {
        dropModelAfterTimeout("Building the model");
        return {false, timeoutMessage_, -1};
    }

    timeoutMessage_.clear();
    if (b.model)
    {
        model_=b.model;
        modelCurrent_=true;
    }
    return b.result;
}


void ISCADHeadlessHost::rebuild(int upToPos, RebuildCallback done)
{
    auto r=build(upToPos);
    done(r.ok, r.message, r.errorPos);
}


void ISCADHeadlessHost::cancelRebuild()
{}


QList<CommandDoc> ISCADHeadlessHost::commandDocumentation() const
{
    return ISCADModelInspection::commandDocumentation();
}


QStringList ISCADHeadlessHost::librarySearchPaths() const
{
    return ISCADModelInspection::librarySearchPaths();
}


ISCADAssistantHost::SymbolTable ISCADHeadlessHost::notBuilt() const
{
    SymbolTable t;
    t.error = !timeoutMessage_.isEmpty() ? timeoutMessage_ :
        model_ ?
        "The script was changed after the last rebuild. Call rebuild first." :
        "No model has been built yet. Call rebuild first.";
    return t;
}


ISCADAssistantHost::SymbolTable ISCADHeadlessHost::symbols()
{
    if (!model_ || !modelCurrent_)
        return notBuilt();
    auto m=model_;
    SymbolTable t;
    if (!runLimited<SymbolTable>(timeLimit_, [m]() { return ISCADModelInspection::symbols(*m); }, t))
    {
        dropModelAfterTimeout("Listing the model symbols");
        return notBuilt();
    }
    return t;
}


ISCADAssistantHost::FeatureProperties ISCADHeadlessHost::featureProperties(const QString& name)
{
    if (!model_ || !modelCurrent_)
    {
        FeatureProperties p;
        p.error=notBuilt().error;
        return p;
    }
    auto m=model_;
    FeatureProperties p;
    if (!runLimited<FeatureProperties>(timeLimit_,
            [m, name]() { return ISCADModelInspection::featureProperties(*m, name); }, p))
    {
        dropModelAfterTimeout(QString("Inspecting feature %1").arg(name));
        FeatureProperties pe;
        pe.error=timeoutMessage_;
        return pe;
    }
    return p;
}
