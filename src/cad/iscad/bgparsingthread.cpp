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
    std::istringstream is(script_);

    int failloc=-1;

    insight::cad::cache.initRebuild();

    insight::cad::ModelPtr oldmodel = model_;
    model_.reset(new insight::cad::Model);
    bool r=false;

    std::string reason="Failed: Syntax error";
    std::cout<<script_;
    try
    {
        r=insight::cad::parseISCADModelStream(is, model_.get(), &failloc, &syn_elem_dir_);
    }
    catch (insight::cad::parser::iscadParserException e)
    {
        reason="Expected: "+e.message();
        failloc=e.from_pos();
        std::cout<<"Error:"<<reason<<std::endl;
        emit scriptError(failloc, QString::fromStdString(reason));
    }

    if (!r) // fail if we did not get a full match
    {
        emit scriptError(failloc, "Syntax error");
    }
    else
    {

        std::cout<<"Parsing done"<<std::endl;

        emit statusMessage("Model parsed successfully.");

        if (action_ >= Rebuild)
        {
            std::cout<<"Building model"<<std::endl;

            {
                // get set with scalar symbols before rebuild (for finding out which have vanished)
                MapDirectory<insight::cad::Model::ScalarTableContents> removedScalars;
                if (oldmodel) removedScalars.set(oldmodel->scalars());

                insight::cad::Model::ScalarTableContents scalars=model_->scalars();
                int is=0, ns=scalars.size();
                BOOST_FOREACH(insight::cad::Model::ScalarTableContents::value_type const& v, scalars)
                {
                    emit statusMessage("Building scalar "+QString::fromStdString(v.first));
//                    v.second->value();
                    std::cout<<v.first<<"="<<v.second->value()<<std::endl; // Trigger evaluation
                    emit statusProgress(is++, ns);
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

                insight::cad::Model::VectorTableContents vectors=model_->vectors();
                int is=0, ns=vectors.size();
                BOOST_FOREACH(insight::cad::Model::VectorTableContents::value_type const& v, vectors)
                {
                    emit statusMessage("Building vector "+QString::fromStdString(v.first));
                    v.second->value(); // Trigger evaluation
                    emit statusProgress(is++, ns);
                    emit createdVariable(QString::fromStdString(v.first), v.second);
                    removedVectors.removeIfPresent(v.first);
                }

                BOOST_FOREACH(const std::string& sn, removedVectors)
                {
                  emit removedVector(QString::fromStdString(sn));
                }
            }

            {
                MapDirectory<insight::cad::Model::DatumTableContents> removedDatums;
                if (oldmodel) removedDatums.set(oldmodel->datums());

                insight::cad::Model::DatumTableContents datums=model_->datums();
                int is=0, ns=datums.size();
                BOOST_FOREACH(insight::cad::Model::DatumTableContents::value_type const& v, datums)
                {
                    emit statusMessage("Building datum "+QString::fromStdString(v.first));
                    v.second->checkForBuildDuringAccess(); // Trigger rebuild
                    emit statusProgress(is++, ns);
                    emit createdDatum(QString::fromStdString(v.first), v.second);
                    removedDatums.removeIfPresent(v.first);
                }

                BOOST_FOREACH(const std::string& sn, removedDatums)
                {
                  emit removedDatum(QString::fromStdString(sn));
                }
            }

            {
                MapDirectory<insight::cad::Model::ModelstepTableContents> removedFeatures;
                if (oldmodel) removedFeatures.set(oldmodel->modelsteps());

                insight::cad::Model::ModelstepTableContents modelsteps=model_->modelsteps();
                int is=0, ns=modelsteps.size();
                BOOST_FOREACH(insight::cad::Model::ModelstepTableContents::value_type const& v, modelsteps)
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
                    emit statusProgress(is++, ns);
                    emit createdFeature(QString::fromStdString(v.first), v.second, is_comp);

                    removedFeatures.removeIfPresent(v.first);
                }

                BOOST_FOREACH(const std::string& sn, removedFeatures)
                {
                  emit removedFeature(QString::fromStdString(sn));
                }

            }

            {
                MapDirectory<insight::cad::Model::PostprocActionTableContents> removedPostprocActions;
                if (oldmodel) removedPostprocActions.set(oldmodel->postprocActions());

                insight::cad::Model::PostprocActionTableContents postprocActions=model_->postprocActions();
                int is=0, ns=postprocActions.size();
                BOOST_FOREACH(insight::cad::Model::PostprocActionTableContents::value_type const& v, postprocActions)
                {
                    emit statusMessage("Building postproc action "+QString::fromStdString(v.first));
                    if (action_ >= Post) v.second->checkForBuildDuringAccess(); // Trigger evaluation
                    emit statusProgress(is++, ns);
                    emit createdEvaluation(QString::fromStdString(v.first), v.second);
                    removedPostprocActions.removeIfPresent(v.first);
                }

                BOOST_FOREACH(const std::string& sn, removedPostprocActions)
                {
                  emit removedEvaluation(QString::fromStdString(sn));
                }
            }

            emit statusMessage("Model rebuild successfully finished.");
        }

        insight::cad::cache.finishRebuild();
    }
}
