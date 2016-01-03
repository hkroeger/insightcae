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
  scalars_[name]=value;
}

void Model::addScalarIfNotPresent(const std::string& name, ScalarPtr value)
{
  if (scalars_.find(name)==scalars_.end())
    addScalar(name, value);
}

void Model::addVector(const std::string& name, VectorPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding vector variable "<<name<<std::endl;
#endif
  vectors_[name]=value;
}

void Model::addVectorIfNotPresent(const std::string& name, VectorPtr value)
{
  if (vectors_.find(name)==vectors_.end())
    addVector(name, value);
}

void Model::addDatum(const std::string& name, DatumPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding datum "<<name<<std::endl;
#endif
  datums_[name]=value;
}

void Model::addModelstep(const std::string& name, FeaturePtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding model step "<<name<<std::endl;
#endif
  modelsteps_[name]=value;
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
  vertexFeatures_[name]=value;
}

void Model::addEdgeFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding edge feature set "<<name<<std::endl;
#endif
  edgeFeatures_[name]=value;
}

void Model::addFaceFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding face feature set "<<name<<std::endl;
#endif
  faceFeatures_[name]=value;
}

void Model::addSolidFeature(const std::string& name, FeatureSetPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding solid feature set "<<name<<std::endl;
#endif
  solidFeatures_[name]=value;
}

void Model::addModel(const std::string& name, ModelPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding model "<<name<<std::endl;
#endif
  models_[name]=value;
}

void Model::addPostprocAction(const std::string& name, PostprocActionPtr value)
{
#ifdef INSIGHT_CAD_DEBUG
  std::cout<<"adding postproc action "<<name<<std::endl;
#endif
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
  }
  while (postprocActions_.find(name)!=postprocActions_.end());
  postprocActions_[name]=value;
  return name;
}


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
}
// solidmodel import(const boost::filesystem::path& filepath)
// {
//   cout << "reading model "<<filepath<<endl;
//   return solidmodel(new SolidModel(filepath));
// }

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
