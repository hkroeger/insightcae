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
	cout<<"insert scalar "<<name<<" = "<<(*sv)->value()<<endl;
    }
    else if ( const VectorPtr* vv = boost::get<VectorPtr>( &boost::fusion::at_c<1>(s) ) )
    {
        addVector(name, *vv);
	cout<<"insert vector "<<name<<" = "<<(*vv)->value()<<endl;
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
: modelname_(modelname)
{
  defaultVariables();  
  copyVariables(vars);
}

void Model::build()
{
  std::string name=modelname_+".iscad";
  
  int failloc=-1;
  if (!parseISCADModelFile(parser::sharedModelFilePath(name), this, &failloc))
  {
    throw insight::Exception("Failed to parse model "+name+
	    str(format(". Stopped at %d.")%failloc));
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
//   scalars_[name]=value;
  scalars_.add(name, value);
}

void Model::addScalarIfNotPresent(const std::string& name, ScalarPtr value)
{
//   if (scalars_.find(name)==scalars_.end())
  if (!scalars_.find(name))
    addScalar(name, value);
}

void Model::addVector(const std::string& name, VectorPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding vector variable "<<name<<std::endl;
#endif
//   vectors_[name]=value;
  vectors_.add(name, value);
}

void Model::addVectorIfNotPresent(const std::string& name, VectorPtr value)
{
//   if (vectors_.find(name)==vectors_.end())
  if (!vectors_.find(name))
    addVector(name, value);
}

void Model::addDatum(const std::string& name, DatumPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding datum "<<name<<std::endl;
#endif
//   datums_[name]=value;
  datums_.add(name, value);
}

void Model::addModelstep(const std::string& name, FeaturePtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding model step "<<name<<std::endl;
#endif
//   modelsteps_[name]=value;
  modelsteps_.add(name, value);
}

void Model::addComponent(const std::string& name, FeaturePtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding component "<<name<<std::endl;
#endif
  components_.insert(name);
  addModelstep(name, value);
}

void Model::addVertexFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding vertex feature set "<<name<<std::endl;
#endif
//   vertexFeatures_[name]=value;
  vertexFeatures_.add(name, value);
}

void Model::addEdgeFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding edge feature set "<<name<<std::endl;
#endif
//   edgeFeatures_[name]=value;
  edgeFeatures_.add(name, value);
}

void Model::addFaceFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding face feature set "<<name<<std::endl;
#endif
//   faceFeatures_[name]=value;
  faceFeatures_.add(name, value);
}

void Model::addSolidFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding solid feature set "<<name<<std::endl;
#endif
//   solidFeatures_[name]=value;
  solidFeatures_.add(name, value);
}

void Model::addModel(const std::string& name, ModelPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding model "<<name<<std::endl;
#endif
//   models_[name]=value;
  models_.add(name, value);
}

void Model::addPostprocAction(const std::string& name, PostprocActionPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding postproc action "<<name<<std::endl;
#endif
//   postprocActions_[name]=value;
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
  }
//   while (postprocActions_.find(name)!=postprocActions_.end());
  while (postprocActions_.find(name));
//   postprocActions_[name]=value;
  postprocActions_.add(name, value);
  return name;
}

ScalarPtr Model::lookupScalar(const std::string& name) const
{
//     ScalarTable::const_iterator it=scalars_.find(name);
//     if (it==scalars_.end())
//       throw insight::Exception("Could not lookup scalar "+name);
//     return it->second;
  
  ScalarPtr *obj = scalars_.find(name);
  if (!obj)
    throw insight::Exception("Could not lookup scalar "+name);
  return *obj;
}

VectorPtr Model::lookupVector(const std::string& name) const
{
//     VectorTable::const_iterator it=vectors_.find(name);
//     if (it==vectors_.end())
//       throw insight::Exception("Could not lookup vector "+name);
//     return it->second;

  VectorPtr *obj = vectors_.find(name);
  if (!obj)
    throw insight::Exception("Could not lookup vector "+name);
  return *obj;
}

DatumPtr Model::lookupDatum(const std::string& name) const
{
//     DatumTable::const_iterator it=datums_.find(name);
//     if (it==datums_.end())
//       throw insight::Exception("Could not lookup datum "+name);
//     return it->second;

  DatumPtr *obj = datums_.find(name);
  if (!obj)
    throw insight::Exception("Could not lookup datum "+name);
  return *obj;
}

FeaturePtr Model::lookupModelstep(const std::string& name) const
{
//     ModelstepTable::const_iterator it=modelsteps_.find(name);
//     if (it==modelsteps_.end())
//       throw insight::Exception("Could not lookup model step "+name);
//     return it->second;

  FeaturePtr *obj = modelsteps_.find(name);
  if (!obj)
    throw insight::Exception("Could not lookup model step "+name);
  return *obj;    
}

FeatureSetPtr Model::lookupVertexFeature(const std::string& name) const
{
//     VertexFeatureTable::const_iterator it=vertexFeatures_.find(name);
//     if (it==vertexFeatures_.end())
//       throw insight::Exception("Could not lookup vertex feature "+name);
//     return it->second;

  FeatureSetPtr *obj = vertexFeatures_.find(name);
  if (!obj)
    throw insight::Exception("Could not lookup vertex feature "+name);
  return *obj;    
}

FeatureSetPtr Model::lookupEdgeFeature(const std::string& name) const
{
//     EdgeFeatureTable::const_iterator it=edgeFeatures_.find(name);
//     if (it==edgeFeatures_.end())
//       throw insight::Exception("Could not lookup edge feature "+name);
//     return it->second;

  FeatureSetPtr *obj = edgeFeatures_.find(name);
  if (!obj)
    throw insight::Exception("Could not lookup edge feature "+name);
  return *obj;    
}

FeatureSetPtr Model::lookupFaceFeature(const std::string& name) const
{
//     FaceFeatureTable::const_iterator it=faceFeatures_.find(name);
//     if (it==faceFeatures_.end())
//       throw insight::Exception("Could not lookup face feature "+name);
//     return it->second;

  FeatureSetPtr *obj = faceFeatures_.find(name);
  if (!obj)
    throw insight::Exception("Could not lookup face feature "+name);
  return *obj;    
}

FeatureSetPtr Model::lookupSolidFeature(const std::string& name) const
{
//     SolidFeatureTable::const_iterator it=solidFeatures_.find(name);
//     if (it==solidFeatures_.end())
//       throw insight::Exception("Could not lookup solid feature "+name);
//     return it->second;

  FeatureSetPtr *obj = solidFeatures_.find(name);
  if (!obj)
    throw insight::Exception("Could not lookup solid feature "+name);
  return *obj;    
}

ModelPtr Model::lookupModel(const std::string& name) const
{
//     ModelTable::const_iterator it=models_.find(name);
//     if (it==models_.end())
//       throw insight::Exception("Could not lookup model "+name);
//     return it->second;

  ModelPtr *obj = models_.find(name);
  if (!obj)
    throw insight::Exception("Could not lookup model "+name);
  return *obj;    
}

PostprocActionPtr Model::lookupPostprocActionSymbol(const std::string& name) const
{
//     PostprocActionTable::const_iterator it=postprocActions_.find(name);
//     if (it==postprocActions_.end())
//       throw insight::Exception("Could not lookup evaluation "+name);
//     return it->second;

  PostprocActionPtr *obj = postprocActions_.find(name);
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

/*
mapkey_parser::mapkey_parser<ScalarPtr> Model::scalarSymbols() const 
{
  return mapkey_parser::mapkey_parser<ScalarPtr>(scalars_); 
}
mapkey_parser::mapkey_parser<VectorPtr> Model::vectorSymbols() const 
{
  return mapkey_parser::mapkey_parser<VectorPtr>(vectors_); 
}
mapkey_parser::mapkey_parser<DatumPtr> Model::datumSymbols() const 
{
  return mapkey_parser::mapkey_parser<DatumPtr>(datums_); 
}
mapkey_parser::mapkey_parser<FeaturePtr> Model::modelstepSymbols() const 
{
  return mapkey_parser::mapkey_parser<FeaturePtr>(modelsteps_); 
}
mapkey_parser::mapkey_parser<FeatureSetPtr> Model::vertexFeatureSymbols() const 
{
  return mapkey_parser::mapkey_parser<FeatureSetPtr>(vertexFeatures_); 
}
mapkey_parser::mapkey_parser<FeatureSetPtr> Model::edgeFeatureSymbols() const 
{
  return mapkey_parser::mapkey_parser<FeatureSetPtr>(edgeFeatures_); 
}
mapkey_parser::mapkey_parser<FeatureSetPtr> Model::faceFeatureSymbols() const 
{
  return mapkey_parser::mapkey_parser<FeatureSetPtr>(faceFeatures_); 
}
mapkey_parser::mapkey_parser<FeatureSetPtr> Model::solidFeatureSymbols() const 
{
  return mapkey_parser::mapkey_parser<FeatureSetPtr>(solidFeatures_); 
}
mapkey_parser::mapkey_parser<ModelPtr> Model::modelSymbols() const 
{
  return mapkey_parser::mapkey_parser<ModelPtr>(models_); 
}*/

// solidmodel import(const boost::filesystem::path& filepath)
// {
//   cout << "reading model "<<filepath<<endl;
//   return solidmodel(new SolidModel(filepath));
// }



SymbolTableContents<ScalarPtr> Model::scalars() const
{
  SymbolTableContents<ScalarPtr> result;
  scalars_.for_each(result);
  return result;
}

SymbolTableContents<VectorPtr> Model::vectors() const
{
  SymbolTableContents<VectorPtr> result;
  vectors_.for_each(result);
  return result;
}

SymbolTableContents<DatumPtr> 		Model::datums() const
{
  SymbolTableContents<DatumPtr> result;
  datums_.for_each(result);
  return result;
}

SymbolTableContents<FeaturePtr> 	Model::modelsteps() const
{
  SymbolTableContents<FeaturePtr> result;
  modelsteps_.for_each(result);
  return result;
}

SymbolTableContents<FeatureSetPtr> 	Model::vertexFeatures() const
{
  SymbolTableContents<FeatureSetPtr> result;
  vertexFeatures_.for_each(result);
  return result;
}

SymbolTableContents<FeatureSetPtr> 	Model::edgeFeatures() const
{
  SymbolTableContents<FeatureSetPtr> result;
  edgeFeatures_.for_each(result);
  return result;
}

SymbolTableContents<FeatureSetPtr> 	Model::faceFeatures() const
{
  SymbolTableContents<FeatureSetPtr> result;
  faceFeatures_.for_each(result);
  return result;
}

SymbolTableContents<FeatureSetPtr> 	Model::solidFeatures() const
{
  SymbolTableContents<FeatureSetPtr> result;
  solidFeatures_.for_each(result);
  return result;
}

SymbolTableContents<ModelPtr> 		Model::models() const
{
  SymbolTableContents<ModelPtr> result;
  models_.for_each(result);
  return result;
}

SymbolTableContents<PostprocActionPtr> 	Model::postprocActions() const
{
  SymbolTableContents<PostprocActionPtr> result;
  postprocActions_.for_each(result);
  return result;
}

arma::mat Model::modelCoG()
{
//   double mtot=0.0;
//   arma::mat cog=vec3(0,0,0);
//   BOOST_FOREACH(const std::string cn, components_)
//   {
//     const SolidModel& m = *(modelstepSymbols_.find(cn)->second);
//     mtot+=m.mass();
//     cog += m.modelCoG()*m.mass();
//   }
//   
//   cout<<"total mass="<<mtot<<endl;
//   
//   if (mtot<1e-10)
//     throw insight::Exception("Total mass is zero!");
//   
//   cog/=mtot;
//   
//   cout<<"CoG="<<cog<<endl;
//   
//   return cog;
  return arma::mat();
}

arma::mat Model::modelBndBox(double deflection) const
{
  return arma::mat();
}

}
}
