#include "iqiscadscriptmodelgenerator.h"

#include "cadfeature.h"
#include "datum.h"

insight::cad::parser::SyntaxElementDirectoryPtr
IQISCADScriptModelGenerator::generate(const std::string& script, Task finalTask)
{
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;
    auto model_=std::make_shared<insight::cad::Model>();


    std::istringstream instream(script);

    int failloc=-1;


    bool r=false;

    try
    {
      std::string reason="Failed: Syntax error";

      try
      {
          r=insight::cad::parseISCADModelStream(instream, model_.get(), &failloc, &syn_elem_dir_);
      }
      catch (const insight::cad::parser::iscadParserException& e)
      {
          reason="Expected: "+e.message();
          failloc=e.from_pos();
          Q_EMIT scriptError(finalTask >= Rebuild ? failloc : -1, QString::fromStdString(reason), 1);
      }

      if (!r) // fail if we did not get a full match
      {
          Q_EMIT scriptError(finalTask >= Rebuild ? failloc : -1, "Syntax error", 1);
      }
      else
      {

          Q_EMIT statusMessage("Model parsed successfully.");

          if (finalTask >= Rebuild)
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
                              insight::cad::VectorVariableType::Point);
              }

              for (const auto& d: directions)
              {
                  Q_EMIT statusMessage("Building vector "+QString::fromStdString(d.first));
                  d.second->value(); // Trigger evaluation
                  Q_EMIT statusProgress(is++, istepmax);
                  Q_EMIT createdVariable(
                              QString::fromStdString(d.first), d.second,
                              insight::cad::VectorVariableType::Direction);
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
          else
          {
              Q_EMIT statusMessage("Model parsed successfully.");
          }
      }
    }
    catch (const insight::cad::CADException& e)
    {
      auto loc=syn_elem_dir_->findElement(e.feature());
      auto fn = loc.first;
      auto p = loc.second;
      Q_EMIT scriptError( p.first, QString::fromStdString(e.as_string()), p.second-p.first);
    }
    catch (const insight::cad::RebuildCancelException& e)
    {
      Q_EMIT statusMessage("Model rebuild cancelled");
    }
    catch (const insight::Exception& e)
    {
      Q_EMIT scriptError(-1, QString::fromStdString(e.as_string()), 0 );
    }

    return syn_elem_dir_;
}


