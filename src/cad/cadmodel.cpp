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

#include "base/exception.h"
#include "base/tools.h"
#include "cadmodel.h"
#include "cadparameters.h"
#include "cadfeature.h"
#include "datum.h"
#include "parser.h"

#include "base/boost_include.h"
#include <memory>

#include "cadfeatures/sphere.h"

using namespace boost;
using namespace std;

namespace insight
{
namespace cad
{


std::ostream& operator<<(std::ostream& os, const Model& m)
{
    os << "Scalars:\n";
    for (auto & s: m.scalars())
    {
        os << " " << s.first << "=" << s.second->value() << "\n";
    }

    os << "Points:\n";
    for (auto & p: m.points())
    {
        os << " " << p.first << "=" << p.second->value().t();
    }

    os << "Directions:\n";
    for (auto & d: m.directions())
    {
        os << " " << d.first << "=" << d.second->value().t();
    }

    os << "Datums:\n";
    for (auto & d: m.datums())
    {
        os << " " << d.first << "\n";
    }

    os << "Modelsteps:\n";
    for (auto & d: m.modelsteps())
    {
        os << " " << d.first << " (" << d.second->type() << ")\n";
        if (auto s=std::dynamic_pointer_cast<Sphere>(d.second))
        {
            s->print(os);
        }
    }

    return os;
}



void Model::defaultVariables()
{
    addScalarIfNotPresent   ( "M_PI", 	ScalarPtr(new ConstantScalar(M_PI)));
    addScalarIfNotPresent   ( "deg", 	ScalarPtr(new ConstantScalar(M_PI/180.)));

    addPointIfNotPresent    ( "O",      VectorPtr(new ConstantVector(vec3(0,0,0))) );
    addDirectionIfNotPresent( "EX", 	VectorPtr(new ConstantVector(vec3(1,0,0))) );
    addDirectionIfNotPresent( "EY", 	VectorPtr(new ConstantVector(vec3(0,1,0))) );
    addDirectionIfNotPresent( "EZ", 	VectorPtr(new ConstantVector(vec3(0,0,1))) );
    addDatumIfNotPresent    ( "XY", 	DatumPtr(new DatumPlane(lookupPoint("O"), lookupDirection("EZ"), lookupDirection("EY"))) );
    addDatumIfNotPresent    ( "XZ", 	DatumPtr(new DatumPlane(lookupPoint("O"), lookupDirection("EY"), lookupDirection("EX"))) );
    addDatumIfNotPresent    ( "YZ", 	DatumPtr(new DatumPlane(lookupPoint("O"), lookupDirection("EX"), lookupDirection("EY"))) );
}

void Model::copyVariables(const ModelVariableTable& vars)
{
    for (const ModelVariableTable::value_type& s: vars)
    {
        const std::string& name=boost::fusion::at_c<0>(s);
        if ( const ScalarPtr* sv = boost::get<ScalarPtr>( &boost::fusion::at_c<1>(s) ) )
        {
            addScalar(name, *sv);
        }
        else if ( const VectorPtrAndType* vv = boost::get<VectorPtrAndType>( &boost::fusion::at_c<1>(s) ) )
        {
          if (boost::fusion::at_c<1>(*vv) == Point)
          {
            addPoint(name, boost::fusion::at_c<0>(*vv) );
          }
          else if (boost::fusion::at_c<1>(*vv) == Direction)
          {
            addDirection(name, boost::fusion::at_c<0>(*vv) );
          }
        }
        else if ( const auto* dd = boost::get<DatumPtr>( &boost::fusion::at_c<1>(s) ) )
        {
            addDatum(name, *dd);
        }
        else if ( const FeaturePtr* ff = boost::get<FeaturePtr>( &boost::fusion::at_c<1>(s) ) )
        {
            addModelstep(name, *ff, false);
        }
        else
            throw insight::UnhandledSelection();
    }
}


size_t Model::calcHash() const
{
  ParameterListHash h;
  h+=modelfile_;
  if (description_) h+=description_->toString();
  h+=cost_;
#warning check, if hash is needed
  return h.getHash();
}



Model::Model(const Model& o, TreeCloneMap& tcm)
  : description_(o.description_),
    cost_(o.cost_),
    components_(o.components_),
#warning implement dep source API
    // syn_elem_dir_(o.syn_elem_dir_)
    modelfile_(o.modelfile_)
{
    tcm.clone(o.scalars_, scalars_);
    tcm.clone(o.points_, points_);
    tcm.clone(o.directions_, directions_);
    tcm.clone(o.datums_, datums_);
    tcm.clone(o.modelsteps_, modelsteps_);
    tcm.clone(o.vertexFeatures_, vertexFeatures_);
    tcm.clone(o.edgeFeatures_, edgeFeatures_);
    tcm.clone(o.faceFeatures_, faceFeatures_);
    tcm.clone(o.solidFeatures_, solidFeatures_);
    tcm.clone(o.models_, models_);
    tcm.clone(o.postprocActions_, postprocActions_);

#warning implement
    //datasets_;
}

defineType(Model);


Model::Model(const ModelVariableTable& vars)
  : cost_(0.0)
{
    copyVariables(vars);
    defaultVariables();

    setValid();
}




Model::Model(const std::string& modelname, const ModelVariableTable& vars)
    : modelfile_(sharedModelFilePath(modelname+".iscad")),
      cost_(0.)
{
    copyVariables(vars);
    defaultVariables();
}



Model::Model(const boost::filesystem::path& modelfile, const ModelVariableTable& vars)
    : modelfile_(modelfile),
      cost_(0.0)
{
#warning local extension of search path required
    insight::SharedPathList::global().insertFileDirectoyIfNotPresent(modelfile_);
    copyVariables(vars);
    defaultVariables();
}


void Model::setDescription(
    BOMDescriptionDataPtr arg )
{
    description_=arg;
}


void Model::setCost(double cost)
{
  cost_=cost;
}

ModelVariableTable Model::allVariables() const
{
    ModelVariableTable mvt;

    //scalarSymbols().for_each(
        //[&](const std::string& name, ScalarPtr v)
    std::for_each(
        scalars().begin(), scalars().end(),
        [&](const ScalarTableContents::value_type& v)
        {
            mvt.push_back(/*ModelVariableTable::value_type{name, */ v /*}*/);
        });

    std::for_each(
        points().begin(), points().end(),
        [&](const VectorTableContents::value_type& v)
        {
            mvt.push_back(ModelVariableTable::value_type{v.first, VectorPtrAndType{v.second, Point}});
        });

    std::for_each(
        directions().begin(), directions().end(),
        [&](const VectorTableContents::value_type& v)
        {
            mvt.push_back(ModelVariableTable::value_type{v.first, VectorPtrAndType{v.second, Direction}});
        });

    std::for_each(
        datums().begin(), datums().end(),
        [&](const DatumTableContents::value_type& v)
        {
            mvt.push_back(v);
        });

    //modelstepSymbols().for_each(
    std::for_each(
        modelsteps().begin(), modelsteps().end(),
        [&](const ModelVariableTable::value_type& v)
        {
            mvt.push_back(v);
        });

    return mvt;
}


void Model::build()
{
    if (!modelfile_.empty())
    {
        ExecTimer t("Model::build() [file "+modelfile_.string()+"]");

        int failloc=-1;
        if (!parseISCADModelFile(modelfile_, this, &failloc, &syn_elem_dir_))
        {
            throw insight::Exception
            (
                "Failed to parse model "
                +modelfile_.string()+
                str(format(". Stopped at %d.")%failloc)
            );
        }
    }
    setValid();
}


void Model::addScalar(const std::string& name, ScalarPtr value)
{
  // if (scalars_.find(name)) scalars_.remove(name);
  // scalars_.add(name, value);
  scalars_[name]=value;
}

void Model::addScalarIfNotPresent(const std::string& name, ScalarPtr value)
{
  if (!scalars_.count(name))
    addScalar(name, value);
}

void Model::addPoint(const std::string& name, VectorPtr value)
{
  points_[name]=value;
}

void Model::addPointIfNotPresent(const std::string& name, VectorPtr value)
{
  if (!points_.count(name))
    addPoint(name, value);
}

void Model::addDirection(const std::string& name, VectorPtr value)
{
  directions_[name]=value;
}

void Model::addDirectionIfNotPresent(const std::string& name, VectorPtr value)
{
  if (!directions_.count(name))
    addDirection(name, value);
}

void Model::addDatum(const std::string& name, DatumPtr value)
{
  datums_[name]=value;
}

void Model::addDatumIfNotPresent(const std::string& name, DatumPtr value)
{
    if (!datums_.count(name))
        addDatum(name, value);
}




void Model::addModelstep(
    const std::string& name,
    FeaturePtr value,
    bool isComponent,
    const std::string& /*featureDescription*/)
{
  value->setFeatureSymbolName(name);

  if (components_.find(name)!=components_.end())
  {
      components_.erase(name);
  }
  if (isComponent)
  {
      components_.insert(name);
  }

  modelsteps_[name]=value;
}




void Model::addModelstepIfNotPresent(
    const std::string& name,
    FeaturePtr value,
    bool isComponent,
    const std::string& /*featureDescription*/)
{
    if (!modelsteps_.count(name))
    {
        addModelstep(name, value, isComponent);
    }
}




// void Model::addComponent(const std::string& name, FeaturePtr value, const std::string& /*featureDescription*/)
// {
//   components_.insert(name);
//   addModelstep(name, value);
// }




void Model::removeScalar(const string& name)
{
  scalars_.erase(name);
}

void Model::removePoint(const std::string& name)
{
  points_.erase(name);
}

void Model::removeDirection(const std::string& name)
{
  directions_.erase(name);
}

void Model::removeDatum(const std::string& name)
{
  datums_.erase(name);
}

void Model::removeModelstep(const std::string& name)
{
  modelsteps_.erase(name);
}



void Model::addVertexFeature(const std::string& name, FeatureSetPtr value)
{
  vertexFeatures_[name]=value;
}

void Model::addEdgeFeature(const std::string& name, FeatureSetPtr value)
{
  edgeFeatures_[name]=value;
}

void Model::addFaceFeature(const std::string& name, FeatureSetPtr value)
{
  faceFeatures_[name]=value;
}

void Model::addSolidFeature(const std::string& name, FeatureSetPtr value)
{
  solidFeatures_[name]=value;
}

void Model::addModel(const std::string& name, ModelPtr value)
{
  models_[name]=value;
}

void Model::addPostprocAction(const std::string& name, PostprocActionPtr value)
{
  postprocActions_[name]=value;
}

std::string Model::addPostprocActionUnnamed(PostprocActionPtr value)
{
  std::string name, nametempl="PostprocAction%d";
  
  int i=1;
  do
  {
    name=str(format(nametempl)%i);
    if (i>1000)
      throw insight::Exception("Model::addPostprocActionUnnamed: No valid name found within 1000 attempts!");
    i++;
  }
  while (postprocActions_.count(name));
  
  postprocActions_.insert({name, value});
  return name;
}

void Model::removePostprocAction(const std::string &name)
{
    postprocActions_.erase(name);
}


void Model::addDataset(const std::string& name, vtkSmartPointer<vtkDataObject> value)
{
    datasets_[name]=value;
}

void Model::removeDataset(const std::string &name)
{
    datasets_.erase(name);
}


ScalarPtr Model::lookupScalar(const std::string& name) const
{
  //ScalarPtr *obj = const_cast<ScalarPtr*>(scalars_.find(name));
  auto obj=scalars_.find(name);
  if (obj==scalars_.end())
    throw insight::Exception("Could not lookup scalar "+name);
  return obj->second;
}

VectorPtr Model::lookupPoint(const std::string& name) const
{
  // VectorPtr *obj = const_cast<VectorPtr*>(points_.find(name));
  // if (!obj)
  auto obj=points_.find(name);
  if (obj==points_.end())
    throw insight::Exception("Could not lookup point "+name);
  return obj->second;
}

VectorPtr Model::lookupDirection(const std::string& name) const
{
  // VectorPtr *obj = const_cast<VectorPtr*>(directions_.find(name));
  // if (!obj)
  auto obj=directions_.find(name);
  if (obj==directions_.end())
    throw insight::Exception("Could not lookup direction "+name);
  return obj->second;
}

DatumPtr Model::lookupDatum(const std::string& name) const
{
  // DatumPtr *obj = const_cast<DatumPtr*>(datums_.find(name));
  // if (!obj)
  auto obj=datums_.find(name);
  if (obj==datums_.end())
    throw insight::Exception("Could not lookup datum "+name);
  return obj->second;
}

FeaturePtr Model::lookupModelstep(const std::string& name) const
{
  auto obj=modelsteps_.find(name);
  if (obj==modelsteps_.end())
    throw insight::Exception("Could not lookup model step "+name);
  return obj->second;
}

FeatureSetPtr Model::lookupVertexFeature(const std::string& name) const
{
  // FeatureSetPtr *obj = const_cast<FeatureSetPtr*>(vertexFeatures_.find(name));
  // if (!obj)
  auto obj=vertexFeatures_.find(name);
  if (obj==vertexFeatures_.end())
      throw insight::Exception("Could not lookup vertex feature "+name);
  return obj->second;
}

FeatureSetPtr Model::lookupEdgeFeature(const std::string& name) const
{
  // FeatureSetPtr *obj = const_cast<FeatureSetPtr*>(edgeFeatures_.find(name));
  // if (!obj)
  auto obj=edgeFeatures_.find(name);
  if (obj==edgeFeatures_.end())
    throw insight::Exception("Could not lookup edge feature "+name);
  return obj->second;
}

FeatureSetPtr Model::lookupFaceFeature(const std::string& name) const
{
  // FeatureSetPtr *obj = const_cast<FeatureSetPtr*>(faceFeatures_.find(name));
  // if (!obj)
  auto obj=faceFeatures_.find(name);
  if (obj==faceFeatures_.end())
    throw insight::Exception("Could not lookup face feature "+name);
  return obj->second;
}

FeatureSetPtr Model::lookupSolidFeature(const std::string& name) const
{
  // FeatureSetPtr *obj = const_cast<FeatureSetPtr*>(solidFeatures_.find(name));
  // if (!obj)
  auto obj=solidFeatures_.find(name);
  if (obj==solidFeatures_.end())
    throw insight::Exception("Could not lookup solid feature "+name);
  return obj->second;
}

ModelPtr Model::lookupModel(const std::string& name) const
{
  // ModelPtr *obj = const_cast<ModelPtr*>(models_.find(name));
  // if (!obj)
  auto obj=models_.find(name);
  if (obj==models_.end())
    throw insight::Exception("Could not lookup model "+name);
  return obj->second;
}

PostprocActionPtr Model::lookupPostprocActionSymbol(const std::string& name) const
{
  // PostprocActionPtr *obj = const_cast<PostprocActionPtr*>(postprocActions_.find(name));
  // if (!obj)
  auto obj=postprocActions_.find(name);
  if (obj==postprocActions_.end())
    throw insight::Exception("Could not lookup postprocessing action "+name);
  return obj->second;
}

bool Model::isComponent(const std::string &name) const
{
    return components_.find(name)!=components_.end();
}

// const Model::ScalarTable& 	Model::scalarSymbols() const { return scalars_; }
// const Model::VectorTable&	Model::pointSymbols() const { return points_; }
// const Model::VectorTable&	Model::directionSymbols() const { return directions_; }
// const Model::DatumTable&	Model::datumSymbols() const { return datums_; }
// const Model::ModelstepTable&	Model::modelstepSymbols() const { return modelsteps_; }
// const Model::VertexFeatureTable&	Model::vertexFeatureSymbols() const { return vertexFeatures_; }
// const Model::EdgeFeatureTable&	Model::edgeFeatureSymbols() const { return edgeFeatures_; }
// const Model::FaceFeatureTable& 	Model::faceFeatureSymbols() const { return faceFeatures_; }
// const Model::SolidFeatureTable& 	Model::solidFeatureSymbols() const { return solidFeatures_; }
// const Model::ModelTable& 	Model::modelSymbols() const { return models_; }
// const Model::PostprocActionTable& 	Model::postprocActionSymbols() const { return postprocActions_; }

const Model::DatasetTable &Model::datasets() const
{
    return datasets_;
}



template<class T>
class SymbolTableContentsInserter
{
  std::map<std::string, T>& tab_;
public:
  SymbolTableContentsInserter(std::map<std::string, T>& tab)
  : tab_(tab)
  {}
  
  void operator()(const std::string& name, T v)
  {
    tab_[name]=v;
  }
};

BOMDescriptionDataPtr Model::description() const
{
  return description_;
}

double Model::cost() const
{
  return cost_;
}

double Model::totalCost() const
{
  double tc = cost();
#warning incomplete!
  return tc;
}

const Model::ScalarTableContents& Model::scalars() const
{
  // ScalarTableContents result;
  // scalars_.for_each(SymbolTableContentsInserter<ScalarPtr>(result));
  // return result;
  return scalars_;
}

const Model::VectorTableContents& Model::points() const
{
  // VectorTableContents result;
  // points_.for_each(SymbolTableContentsInserter<VectorPtr>(result));
  // return result;
  return points_;
}

const Model::VectorTableContents& Model::directions() const
{
  // VectorTableContents result;
  // directions_.for_each(SymbolTableContentsInserter<VectorPtr>(result));
  // return result;
  return directions_;
}

const Model::DatumTableContents& Model::datums() const
{
  // DatumTableContents result;
  // datums_.for_each(SymbolTableContentsInserter<DatumPtr>(result));
  // return result;
  return datums_;
}

const Model::ModelstepTableContents& Model::modelsteps() const
{
  // ModelstepTableContents result;
  // modelsteps_.for_each(SymbolTableContentsInserter<FeaturePtr>(result));
  // return result;
  return modelsteps_;
}

const Model::VertexFeatureTableContents& Model::vertexFeatures() const
{
  // VertexFeatureTableContents result;
  // vertexFeatures_.for_each(SymbolTableContentsInserter<FeatureSetPtr>(result));
  // return result;
  return vertexFeatures_;
}

const Model::EdgeFeatureTableContents& Model::edgeFeatures() const
{
  // EdgeFeatureTableContents result;
  // edgeFeatures_.for_each(SymbolTableContentsInserter<FeatureSetPtr>(result));
  // return result;
  return edgeFeatures_;
}

const Model::FaceFeatureTableContents& Model::faceFeatures() const
{
  // FaceFeatureTableContents result;
  // faceFeatures_.for_each(SymbolTableContentsInserter<FeatureSetPtr>(result));
  // return result;
  return faceFeatures_;
}

const Model::SolidFeatureTableContents& Model::solidFeatures() const
{
  // SolidFeatureTableContents result;
  // solidFeatures_.for_each(SymbolTableContentsInserter<FeatureSetPtr>(result));
  // return result;
  return solidFeatures_;
}

const Model::ModelTableContents& Model::models() const
{
  // ModelTableContents result;
  // models_.for_each(SymbolTableContentsInserter<ModelPtr>(result));
  // return result;
  return models_;
}

const Model::PostprocActionTableContents& Model::postprocActions() const
{
  // PostprocActionTableContents result;
  // postprocActions_.for_each(SymbolTableContentsInserter<PostprocActionPtr>(result));
  // return result;
  return postprocActions_;
}


std::shared_ptr<DependencySource>
Model::shallowClone(TreeCloneMap& tcm) const
{
    return std::shared_ptr<DependencySource>(
        new Model(*this, tcm)
        );
}


}
}
