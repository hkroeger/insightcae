/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "bgparsingthread.h"

#include "cadfeature.h"
#include "cadmodel.h"
#include "datum.h"

#include <thread>

BGParsingThread::BGParsingThread()
: action_(Parse)
{
}




void BGParsingThread::launch(const std::string& script, Action action)
{
    script_=script;
    action_=action;
    start();
}



template<class T>
class MapDirectory
: public std::set<std::string>
{
public:
  MapDirectory()
  {}

  MapDirectory(const T& map)
  {
    set(map);
  }

  void set(const T& map)
  {
    transform
        (
          map.begin(), map.end(),
          inserter( *this, this->begin() ),
          bind(&T::value_type::first, _1 )
        );
  }

  void removeIfPresent(const std::string& key)
  {
    iterator i=find(key);
    if (i!=end())
      {
        erase(i);
      }
  }
};


void BGParsingThread::run()
{
    thread_id_=std::this_thread::get_id();

    std::istringstream is(script_);

    int failloc=-1;


    insight::cad::ModelPtr oldmodel = last_rebuilt_model_;
    model_.reset(new insight::cad::Model);
    bool r=false;

    try
    {
      std::string reason="Failed: Syntax error";

      try
      {
          r=insight::cad::parseISCADModelStream(is, model_.get(), &failloc, &syn_elem_dir_);
      }
      catch (insight::cad::parser::iscadParserException e)
      {
          reason="Expected: "+e.message();
          failloc=e.from_pos();
          emit scriptError(action_ >= Rebuild ? failloc : -1, QString::fromStdString(reason), 1);
      }

      if (!r) // fail if we did not get a full match
      {
          emit scriptError(action_ >= Rebuild ? failloc : -1, "Syntax error", 1);
      }
      else
      {

          emit statusMessage("Model parsed successfully.");

          if (action_ >= Rebuild)
          {
              emit statusMessage("Model parsed successfully, starting rebuild...");

              insight::cad::cache.initRebuild();

              insight::cad::Model::ScalarTableContents scalars=model_->scalars();
              insight::cad::Model::VectorTableContents vectors=model_->vectors();
              insight::cad::Model::ModelstepTableContents modelsteps=model_->modelsteps();
              insight::cad::Model::DatumTableContents datums=model_->datums();
              insight::cad::Model::PostprocActionTableContents postprocActions=model_->postprocActions();

              int is=0;
              int istepmax=scalars.size()+vectors.size()+modelsteps.size()+datums.size() -1;

              {
                  // get set with scalar symbols before rebuild (for finding out which have vanished)
                  MapDirectory<insight::cad::Model::ScalarTableContents> removedScalars;
                  if (oldmodel) removedScalars.set(oldmodel->scalars());

                  BOOST_FOREACH(insight::cad::Model::ScalarTableContents::value_type v, scalars)
                  {
                      emit statusMessage("Building scalar "+QString::fromStdString(v.first));
  //                    v.second->value();
                      std::cout<<v.first<<"="<<v.second->value()<<std::endl; // Trigger evaluation
                      emit statusProgress(is++, istepmax);
                      emit createdVariable(QString::fromStdString(v.first), v.second);
                      removedScalars.removeIfPresent(v.first);
                  }

                  BOOST_FOREACH(const std::string& sn, removedScalars)
                  {
                    emit removedScalar(QString::fromStdString(sn));
                  }
              }


              {
                  MapDirectory<insight::cad::Model::VectorTableContents> removedVectors;
                  if (oldmodel) removedVectors.set(oldmodel->vectors());

                  BOOST_FOREACH(insight::cad::Model::VectorTableContents::value_type v, vectors)
                  {
                      emit statusMessage("Building vector "+QString::fromStdString(v.first));
                      v.second->value(); // Trigger evaluation
                      emit statusProgress(is++, istepmax);
                      emit createdVariable(QString::fromStdString(v.first), v.second);
                      removedVectors.removeIfPresent(v.first);
                  }

                  BOOST_FOREACH(const std::string& sn, removedVectors)
                  {
                    emit removedVector(QString::fromStdString(sn));
                  }
              }

              {
                  MapDirectory<insight::cad::Model::ModelstepTableContents> removedFeatures;
                  if (oldmodel) removedFeatures.set(oldmodel->modelsteps());

                  BOOST_FOREACH(insight::cad::Model::ModelstepTableContents::value_type v, modelsteps)
                  {
                      bool is_comp=false;
                      if (model_->components().find(v.first) != model_->components().end())
                      {
                          is_comp=true;
                          emit statusMessage("Building component "+QString::fromStdString(v.first));
                      } else
                      {
                          emit statusMessage("Building feature "+QString::fromStdString(v.first));
                      }
                      v.second->checkForBuildDuringAccess(); // Trigger rebuild
                      emit statusProgress(is++, istepmax);
                      emit createdFeature(QString::fromStdString(v.first), v.second, is_comp);

                      removedFeatures.removeIfPresent(v.first);
                  }

                  BOOST_FOREACH(const std::string& sn, removedFeatures)
                  {
                    emit removedFeature(QString::fromStdString(sn));
                  }

              }

              {
                  MapDirectory<insight::cad::Model::DatumTableContents> removedDatums;
                  if (oldmodel) removedDatums.set(oldmodel->datums());

                  BOOST_FOREACH(insight::cad::Model::DatumTableContents::value_type v, datums)
                  {
                      emit statusMessage("Building datum "+QString::fromStdString(v.first));
                      v.second->checkForBuildDuringAccess(); // Trigger rebuild
                      emit statusProgress(is++, istepmax);
                      emit createdDatum(QString::fromStdString(v.first), v.second);
                      removedDatums.removeIfPresent(v.first);
                  }

                  BOOST_FOREACH(const std::string& sn, removedDatums)
                  {
                    emit removedDatum(QString::fromStdString(sn));
                  }
              }

              {
                  MapDirectory<insight::cad::Model::PostprocActionTableContents> removedPostprocActions;
                  if (oldmodel) removedPostprocActions.set(oldmodel->postprocActions());

                  int is=0, ns=postprocActions.size();
                  BOOST_FOREACH(insight::cad::Model::PostprocActionTableContents::value_type v, postprocActions)
                  {
                      emit statusMessage("Building postproc action "+QString::fromStdString(v.first));
                      if (action_ >= Post) v.second->checkForBuildDuringAccess(); // Trigger evaluation
                      emit statusProgress(is++, ns-1);
                      emit createdEvaluation(QString::fromStdString(v.first), v.second);
                      removedPostprocActions.removeIfPresent(v.first);
                  }

                  BOOST_FOREACH(const std::string& sn, removedPostprocActions)
                  {
                    emit removedEvaluation(QString::fromStdString(sn));
                  }
              }

              last_rebuilt_model_ = model_;

              insight::cad::cache.finishRebuild();

              emit statusMessage("Model rebuild successfully finished.");
          }
          else
            {
              emit statusMessage("Model parsed successfully.");
            }
      }
    }
    catch (insight::cad::CADException e)
    {
      auto loc=syn_elem_dir_->findElement(e.feature());
      auto fn = loc.first;
      auto p = loc.second;
      emit scriptError( p.first, QString::fromStdString(e.as_string()), p.second-p.first);
    }
    catch (insight::cad::RebuildCancelException e)
    {
      emit statusMessage("Model rebuild cancelled");
    }
    catch (insight::Exception e)
    {
      emit scriptError(-1, QString::fromStdString(e.as_string()), 0 );
    }

}


void BGParsingThread::cancelRebuild()
{
  insight::cad::Feature::cancelRebuild(thread_id_);
}
