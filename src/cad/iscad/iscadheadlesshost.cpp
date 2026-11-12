#include "iscadheadlesshost.h"

#include <sstream>

#include "parser.h"
#include "cadfeature.h"
#include "cadexception.h"
#include "cadmodel.h"
#include "datum.h"
#include "cadpostprocaction.h"

#include "iscadmodelinspection.h"
#include "scriptpositionmap.h"


using namespace insight::assistant;


ISCADHeadlessHost::ISCADHeadlessHost(const QString& script, const QString& fileName)
    : script_(script),
      fileName_(fileName)
{}


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
            return {false, "Parsing of model script failed", failloc>=0 ? pm.toChar(failloc) : -1};
        }
    }
    catch (const insight::cad::parser::iscadParserException& e)
    {
        return {false, QString::fromStdString(e.summary()), pm.toChar(e.from_pos())};
    }
    catch (const insight::Exception& e)
    {
        return {false, QString::fromStdString(e), -1};
    }
    catch (const std::exception& e)
    {
        return {false, QString::fromStdString(e.what()), -1};
    }

    // ===== build everything (as the editor does, without writing postproc output)
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
        model_=model; // partially built: inspection of the built features is possible
        modelCurrent_=true;
        return {false, QString::fromStdString(e), pos};
    }
    catch (const insight::Exception& e)
    {
        model_=model;
        modelCurrent_=true;
        return {false, QString::fromStdString(e), -1};
    }
    catch (const std::exception& e)
    {
        model_=model;
        modelCurrent_=true;
        return {false, QString::fromStdString(e.what()), -1};
    }

    model_=model;
    modelCurrent_=true;
    return {true, QString(), -1};
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
    t.error = model_ ?
        "The script was changed after the last rebuild. Call rebuild first." :
        "No model has been built yet. Call rebuild first.";
    return t;
}


ISCADAssistantHost::SymbolTable ISCADHeadlessHost::symbols()
{
    if (!model_ || !modelCurrent_)
        return notBuilt();
    return ISCADModelInspection::symbols(*model_);
}


ISCADAssistantHost::FeatureProperties ISCADHeadlessHost::featureProperties(const QString& name)
{
    if (!model_ || !modelCurrent_)
    {
        FeatureProperties p;
        p.error=notBuilt().error;
        return p;
    }
    return ISCADModelInspection::featureProperties(*model_, name);
}
