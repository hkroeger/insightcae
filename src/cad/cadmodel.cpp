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

#include "base/tools.h"
#include "cadmodel.h"
#include "datum.h"
#include "parser.h"

#include "base/boost_include.h"

using namespace boost;
using namespace std;

namespace insight
{
namespace cad
{

void Model::defaultVariables()
{
    addScalar( "M_PI", 	ScalarPtr(new ConstantScalar(M_PI)));
    addScalar( "deg", 	ScalarPtr(new ConstantScalar(M_PI/180.)));
    addVector( "O", 	VectorPtr(new ConstantVector(vec3(0,0,0))) );
    addVector( "EX", 	VectorPtr(new ConstantVector(vec3(1,0,0))) );
    addVector( "EY", 	VectorPtr(new ConstantVector(vec3(0,1,0))) );
    addVector( "EZ", 	VectorPtr(new ConstantVector(vec3(0,0,1))) );
    addDatum ( "XY", 	DatumPtr(new DatumPlane(lookupVector("O"), lookupVector("EZ"), lookupVector("EY"))) );
    addDatum ( "XZ", 	DatumPtr(new DatumPlane(lookupVector("O"), lookupVector("EY"), lookupVector("EX"))) );
    addDatum ( "YZ", 	DatumPtr(new DatumPlane(lookupVector("O"), lookupVector("EX"), lookupVector("EY"))) );
}

void Model::copyVariables(const ModelVariableTable& vars)
{
    BOOST_FOREACH(const ModelVariableTable::value_type& s, vars)
    {
        const std::string& name=boost::fusion::at_c<0>(s);
        if ( const ScalarPtr* sv = boost::get<ScalarPtr>( &boost::fusion::at_c<1>(s) ) )
        {
            addScalar(name, *sv);
        }
        else if ( const VectorPtr* vv = boost::get<VectorPtr>( &boost::fusion::at_c<1>(s) ) )
        {
            addVector(name, *vv);
        }
        else if ( const DatumPtr* dd = boost::get<DatumPtr>( &boost::fusion::at_c<1>(s) ) )
        {
            addDatum(name, *dd);
        }
        else if ( const FeaturePtr* ff = boost::get<FeaturePtr>( &boost::fusion::at_c<1>(s) ) )
        {
            addModelstep(name, *ff);
        }
    }
}




Model::Model(const ModelVariableTable& vars)
{
    defaultVariables();
    copyVariables(vars);

    setValid();
}




Model::Model(const std::string& modelname, const ModelVariableTable& vars)
    : modelfile_(sharedModelFilePath(modelname+".iscad"))
{
    defaultVariables();
    copyVariables(vars);
}



Model::Model(const boost::filesystem::path& modelfile, const ModelVariableTable& vars)
    : modelfile_(modelfile)
{
    insight::SharedPathList::searchPathList.insertFileDirectoyIfNotPresent(modelfile_);
    defaultVariables();
    copyVariables(vars);
}



void Model::build()
{
    int failloc=-1;
    if (!parseISCADModelFile(modelfile_, this, &failloc))
    {
        throw insight::Exception
        (
            "Failed to parse model "
            +modelfile_.string()+
            str(format(". Stopped at %d.")%failloc)
        );
    }
    else
    {
        setValid();
    }
}


void Model::addScalar(const std::string& name, ScalarPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding scalar variable "<<name<<std::endl;
#endif
  scalars_.add(name, value);
}

void Model::addScalarIfNotPresent(const std::string& name, ScalarPtr value)
{
  if (!scalars_.find(name))
    addScalar(name, value);
}

void Model::addVector(const std::string& name, VectorPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding vector variable "<<name<<std::endl;
#endif
  vectors_.add(name, value);
}

void Model::addVectorIfNotPresent(const std::string& name, VectorPtr value)
{
  if (!vectors_.find(name))
    addVector(name, value);
}

void Model::addDatum(const std::string& name, DatumPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding datum "<<name<<std::endl;
#endif
  datums_.add(name, value);
}

void Model::addDatumIfNotPresent(const std::string& name, DatumPtr value)
{
    if (!datums_.find(name))
        addDatum(name, value);
}

void Model::addModelstep(const std::string& name, FeaturePtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding model step "<<name<<std::endl;
#endif
  modelsteps_.add(name, value);
}

void Model::addModelstepIfNotPresent(const std::string& name, FeaturePtr value)
{
    if (!modelsteps_.find(name))
        addModelstep(name, value);
}

void Model::addComponent(const std::string& name, FeaturePtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding component "<<name<<std::endl;
#endif
  components_.insert(name);
  addModelstep(name, value);
}

void Model::removeScalar(const string& name)
{
  scalars_.remove(name);
}


void Model::addVertexFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding vertex feature set "<<name<<std::endl;
#endif
  vertexFeatures_.add(name, value);
}

void Model::addEdgeFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding edge feature set "<<name<<std::endl;
#endif
  edgeFeatures_.add(name, value);
}

void Model::addFaceFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding face feature set "<<name<<std::endl;
#endif
  faceFeatures_.add(name, value);
}

void Model::addSolidFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding solid feature set "<<name<<std::endl;
#endif
  solidFeatures_.add(name, value);
}

void Model::addModel(const std::string& name, ModelPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding model "<<name<<std::endl;
#endif
  models_.add(name, value);
}

void Model::addPostprocAction(const std::string& name, PostprocActionPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding postproc action "<<name<<std::endl;
#endif
  postprocActions_.add(name, value);
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
  while (postprocActions_.find(name));
  
  postprocActions_.add(name, value);
  return name;
}

ScalarPtr Model::lookupScalar(const std::string& name) const
{
  ScalarPtr *obj = const_cast<ScalarPtr*>(scalars_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup scalar "+name);
  return *obj;
}

VectorPtr Model::lookupVector(const std::string& name) const
{
  VectorPtr *obj = const_cast<VectorPtr*>(vectors_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup vector "+name);
  return *obj;
}

DatumPtr Model::lookupDatum(const std::string& name) const
{
  DatumPtr *obj = const_cast<DatumPtr*>(datums_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup datum "+name);
  return *obj;
}

FeaturePtr Model::lookupModelstep(const std::string& name) const
{
  FeaturePtr *obj = const_cast<FeaturePtr*>(modelsteps_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup model step "+name);
  return *obj;    
}

FeatureSetPtr Model::lookupVertexFeature(const std::string& name) const
{
  FeatureSetPtr *obj = const_cast<FeatureSetPtr*>(vertexFeatures_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup vertex feature "+name);
  return *obj;    
}

FeatureSetPtr Model::lookupEdgeFeature(const std::string& name) const
{
  FeatureSetPtr *obj = const_cast<FeatureSetPtr*>(edgeFeatures_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup edge feature "+name);
  return *obj;    
}

FeatureSetPtr Model::lookupFaceFeature(const std::string& name) const
{
  FeatureSetPtr *obj = const_cast<FeatureSetPtr*>(faceFeatures_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup face feature "+name);
  return *obj;    
}

FeatureSetPtr Model::lookupSolidFeature(const std::string& name) const
{
  FeatureSetPtr *obj = const_cast<FeatureSetPtr*>(solidFeatures_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup solid feature "+name);
  return *obj;    
}

ModelPtr Model::lookupModel(const std::string& name) const
{
  ModelPtr *obj = const_cast<ModelPtr*>(models_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup model "+name);
  return *obj;    
}

PostprocActionPtr Model::lookupPostprocActionSymbol(const std::string& name) const
{
  PostprocActionPtr *obj = const_cast<PostprocActionPtr*>(postprocActions_.find(name));
  if (!obj)
    throw insight::Exception("Could not lookup postprocessing action "+name);
  return *obj;    
}

const Model::ScalarTable& 	Model::scalarSymbols() const { return scalars_; }
const Model::VectorTable&	Model::vectorSymbols() const { return vectors_; }
const Model::DatumTable&	Model::datumSymbols() const { return datums_; }
const Model::ModelstepTable&	Model::modelstepSymbols() const { return modelsteps_; }
const Model::VertexFeatureTable&	Model::vertexFeatureSymbols() const { return vertexFeatures_; }
const Model::EdgeFeatureTable&	Model::edgeFeatureSymbols() const { return edgeFeatures_; }
const Model::FaceFeatureTable& 	Model::faceFeatureSymbols() const { return faceFeatures_; }
const Model::SolidFeatureTable& 	Model::solidFeatureSymbols() const { return solidFeatures_; }
const Model::ModelTable& 	Model::modelSymbols() const { return models_; }
const Model::PostprocActionTable& 	Model::postprocActionSymbols() const { return postprocActions_; }



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


Model::ScalarTableContents Model::scalars() const
{
  ScalarTableContents result;
  scalars_.for_each(SymbolTableContentsInserter<ScalarPtr>(result));
  return result;
}

Model::VectorTableContents Model::vectors() const
{
  VectorTableContents result;
  vectors_.for_each(SymbolTableContentsInserter<VectorPtr>(result));
  return result;
}

Model::DatumTableContents Model::datums() const
{
  DatumTableContents result;
  datums_.for_each(SymbolTableContentsInserter<DatumPtr>(result));
  return result;
}

Model::ModelstepTableContents Model::modelsteps() const
{
  ModelstepTableContents result;
  modelsteps_.for_each(SymbolTableContentsInserter<FeaturePtr>(result));
  return result;
}

Model::VertexFeatureTableContents Model::vertexFeatures() const
{
  VertexFeatureTableContents result;
  vertexFeatures_.for_each(SymbolTableContentsInserter<FeatureSetPtr>(result));
  return result;
}

Model::EdgeFeatureTableContents Model::edgeFeatures() const
{
  EdgeFeatureTableContents result;
  edgeFeatures_.for_each(SymbolTableContentsInserter<FeatureSetPtr>(result));
  return result;
}

Model::FaceFeatureTableContents Model::faceFeatures() const
{
  FaceFeatureTableContents result;
  faceFeatures_.for_each(SymbolTableContentsInserter<FeatureSetPtr>(result));
  return result;
}

Model::SolidFeatureTableContents Model::solidFeatures() const
{
  SolidFeatureTableContents result;
  solidFeatures_.for_each(SymbolTableContentsInserter<FeatureSetPtr>(result));
  return result;
}

Model::ModelTableContents Model::models() const
{
  ModelTableContents result;
  models_.for_each(SymbolTableContentsInserter<ModelPtr>(result));
  return result;
}

Model::PostprocActionTableContents Model::postprocActions() const
{
  PostprocActionTableContents result;
  postprocActions_.for_each(SymbolTableContentsInserter<PostprocActionPtr>(result));
  return result;
}


}
}
