#include "iqiscadscriptmodelgenerator.h"

#include "cadfeature.h"
#include "cadexception.h"
#include "datum.h"

ISCADParseResultPtr
IQISCADScriptModelGenerator::parse(const std::string& script)
{
    auto res = std::make_shared<ISCADParseResult>();
    res->script = script;
    res->model = std::make_shared<insight::cad::Model>();

    std::istringstream instream(script);
    int failloc=-1;

    try
    {
        res->success=insight::cad::parseISCADModelStream(
            instream, res->model.get(), &failloc, &res->syntaxElements);

        if (!res->success) // fail if we did not get a full match
        {
            res->failpos = failloc;
            res->errorMsg = "Parsing of model script failed";
            res->errorRange = 1;
        }
    }
    catch (const insight::cad::parser::iscadParserException& e)
    {
        res->success = false;
        res->failpos = e.from_pos();
        res->errorMsg = QString::fromStdString(e.summary());
        res->errorRange = std::max(1, e.to_pos()-e.from_pos());
    }
    catch (const insight::Exception& e)
    {
        res->success = false;
        res->failpos = -1;
        res->errorMsg = QString::fromStdString(e);
        res->errorRange = 0;
    }

    if (res->success)
    {
        Q_EMIT statusMessage("Model parsed successfully.");
    }

    return res;
}




void IQISCADScriptModelGenerator::rebuild(const ISCADParseResult& parsed, Task finalTask)
{
    if (!parsed.success)
    {
        Q_EMIT scriptError(
            finalTask >= Rebuild ? parsed.failpos : -1,
            parsed.errorMsg,
            parsed.errorRange );
        return;
    }

    if (finalTask < Rebuild)
    {
        return;
    }

    auto model_ = parsed.model;
    const auto& syn_elem_dir_ = parsed.syntaxElements;

    try
    {
        emit statusMessage("Model parsed successfully, starting rebuild...");

        auto scalars=model_->scalars();
        auto points=model_->points();
        auto directions=model_->directions();
        auto modelsteps=model_->modelsteps();
        auto datums=model_->datums();
        auto postprocActions=model_->postprocActions();

        int is = 0,
            istepmax=
                  scalars.size()
                + points.size()
                + directions.size()
                + modelsteps.size()
                + datums.size()
                + ( finalTask >= Post ? postprocActions.size() : 0 )
                - 1;

        for (const auto& v: scalars)
        {
            Q_EMIT statusMessage("Building scalar "+QString::fromStdString(v.first));
//                    v.second->value();
            std::cout<<v.first<<"="<<v.second->value()<<std::endl; // Trigger evaluation
            Q_EMIT statusProgress(is++, istepmax);
            Q_EMIT createdVariable(QString::fromStdString(v.first), v.second);
        }

        for (const auto& p: points)
        {
            Q_EMIT statusMessage("Building point "+QString::fromStdString(p.first));
            p.second->value(); // Trigger evaluation
            Q_EMIT statusProgress(is++, istepmax);
            Q_EMIT createdVariable(
                        QString::fromStdString(p.first), p.second,
                        insight::cad::VectorVariableType::Point, false);
        }

        for (const auto& d: directions)
        {
            Q_EMIT statusMessage("Building vector "+QString::fromStdString(d.first));
            d.second->value(); // Trigger evaluation
            Q_EMIT statusProgress(is++, istepmax);
            Q_EMIT createdVariable(
                        QString::fromStdString(d.first), d.second,
                        insight::cad::VectorVariableType::Direction, false);
        }

        for (const auto& v: modelsteps)
        {
            bool is_comp=false;
            if (model_->components().find(v.first) != model_->components().end())
            {
                is_comp=true;
                Q_EMIT statusMessage("Building component "+QString::fromStdString(v.first));
            } else
            {
                Q_EMIT statusMessage("Building feature "+QString::fromStdString(v.first));
            }
            v.second->checkForBuildDuringAccess(); // Trigger rebuild
            Q_EMIT statusProgress(is++, istepmax);
            Q_EMIT createdFeature(QString::fromStdString(v.first), v.second, is_comp);
        }

        for (const auto& v: datums)
        {
            Q_EMIT statusMessage("Building datum "+QString::fromStdString(v.first));
            v.second->checkForBuildDuringAccess(); // Trigger rebuild
            Q_EMIT statusProgress(is++, istepmax);
            Q_EMIT createdDatum(QString::fromStdString(v.first), v.second);
        }

        if (finalTask >= Post)
        {
            for (const auto& v: postprocActions)
            {
                Q_EMIT statusMessage("Building postproc action "+QString::fromStdString(v.first));
                 v.second->checkForBuildDuringAccess(); // Trigger evaluation
                Q_EMIT statusProgress(is++, istepmax);
                Q_EMIT createdEvaluation(QString::fromStdString(v.first), v.second, false);
            }
        }

        insight::cad::cache.printSummary(std::cout);

        std::cout << "total cost of model = " << model_->totalCost();

        Q_EMIT statusMessage("Model rebuild successfully finished.");
    }
    catch (const insight::CADException& e)
    {
      auto loc=syn_elem_dir_->findLocation(
            std::const_pointer_cast<insight::cad::Feature>(e.description()->geometryInError_) );
      auto p = loc.second;
      Q_EMIT scriptError( p.first, QString::fromStdString(e), p.second-p.first);
    }
    catch (const insight::cad::RebuildCancelException& e)
    {
      Q_EMIT statusMessage("Model rebuild cancelled");
      throw;
    }
    catch (const insight::Exception& e)
    {
      Q_EMIT scriptError(-1, QString::fromStdString(e), 0 );
    }
}
