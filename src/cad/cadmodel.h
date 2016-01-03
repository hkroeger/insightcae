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

#ifndef INSIGHT_CAD_MODEL_H
#define INSIGHT_CAD_MODEL_H

#include "cadtypes.h"
#include "base/exception.h"
#include "cadparameters.h"
#include "mapkey_parser.h"
#include "astbase.h"

namespace insight 
{
namespace cad 
{

  
class Model
: public ASTBase
{
public:
  
  typedef std::map<std::string, ScalarPtr> 	ScalarTable;
  typedef std::map<std::string, VectorPtr> 	VectorTable;
  typedef std::map<std::string, DatumPtr> 	DatumTable;
  typedef std::map<std::string, FeaturePtr> 	ModelstepTable;
  typedef std::map<std::string, ModelPtr> 	ModelTable;

  typedef std::map<std::string, FeatureSetPtr> 	VertexFeatureTable;
  typedef std::map<std::string, FeatureSetPtr> 	EdgeFeatureTable;
  typedef std::map<std::string, FeatureSetPtr> 	FaceFeatureTable;
  typedef std::map<std::string, FeatureSetPtr> 	SolidFeatureTable;
  typedef std::map<std::string, PostprocActionPtr> 	PostprocActionTable;
  
  typedef std::set<std::string> componentTable;
  
protected:
  ScalarTable 		scalars_;
  VectorTable 		vectors_;
  DatumTable 		datums_;
  ModelstepTable	modelsteps_;
  VertexFeatureTable	vertexFeatures_;
  EdgeFeatureTable	edgeFeatures_;
  FaceFeatureTable	faceFeatures_;
  SolidFeatureTable	solidFeatures_;
  ModelTable		models_;
  PostprocActionTable	postprocActions_;
  componentTable	components_;
  
  std::string modelname_;
  
  
  void defaultVariables();
  void copyVariables(const ModelVariableTable& vars);
  
public:
  
  Model(const ModelVariableTable& vars = ModelVariableTable());
  Model(const std::string& modelname, const ModelVariableTable& vars = ModelVariableTable());
  
  virtual void build();

  mapkey_parser::mapkey_parser<ScalarPtr> 	scalarSymbols() const;
  mapkey_parser::mapkey_parser<VectorPtr> 	vectorSymbols() const;
  mapkey_parser::mapkey_parser<DatumPtr> 	datumSymbols() const;
  mapkey_parser::mapkey_parser<FeaturePtr> 	modelstepSymbols() const;
  mapkey_parser::mapkey_parser<FeatureSetPtr> 	vertexFeatureSymbols() const;
  mapkey_parser::mapkey_parser<FeatureSetPtr> 	edgeFeatureSymbols() const;
  mapkey_parser::mapkey_parser<FeatureSetPtr> 	faceFeatureSymbols() const;
  mapkey_parser::mapkey_parser<FeatureSetPtr> 	solidFeatureSymbols() const;
  mapkey_parser::mapkey_parser<ModelPtr> 	modelSymbols() const;
  mapkey_parser::mapkey_parser<PostprocActionPtr> 	postprocActionSymbols() const;

    
  inline void addScalar(const std::string& name, ScalarPtr value)
  {
    scalars_[name]=value;
  }
  inline void addScalarIfNotPresent(const std::string& name, ScalarPtr value)
  {
    if (scalars_.find(name)==scalars_.end())
      scalars_[name]=value;
  }
  inline void addVector(const std::string& name, VectorPtr value)
  {
    vectors_[name]=value;
  }
  inline void addVectorIfNotPresent(const std::string& name, VectorPtr value)
  {
    if (vectors_.find(name)==vectors_.end())
      vectors_[name]=value;
  }
  inline void addDatum(const std::string& name, DatumPtr value)
  {
    datums_[name]=value;
  }
  inline void addModelstep(const std::string& name, FeaturePtr value)
  {
    modelsteps_[name]=value;
  }
  inline void addVertexFeature(const std::string& name, FeatureSetPtr value)
  {
    vertexFeatures_[name]=value;
  }
  inline void addEdgeFeature(const std::string& name, FeatureSetPtr value)
  {
    edgeFeatures_[name]=value;
  }
  inline void addFaceFeature(const std::string& name, FeatureSetPtr value)
  {
    faceFeatures_[name]=value;
  }
  inline void addSolidFeature(const std::string& name, FeatureSetPtr value)
  {
    solidFeatures_[name]=value;
  }
  inline void addModel(const std::string& name, ModelPtr value)
  {
    models_[name]=value;
  }
  inline void addPostprocAction(const std::string& name, PostprocActionPtr value)
  {
    postprocActions_[name]=value;
  }
  
  std::string addPostprocActionUnnamed(PostprocActionPtr value);
  
  inline ScalarPtr lookupScalar(const std::string& name) const
  {
    ScalarTable::const_iterator it=scalars_.find(name);
    if (it==scalars_.end())
      throw insight::Exception("Could not lookup scalar "+name);
    return it->second;
  }
  inline VectorPtr lookupVector(const std::string& name) const
  {
    VectorTable::const_iterator it=vectors_.find(name);
    if (it==vectors_.end())
      throw insight::Exception("Could not lookup vector "+name);
    return it->second;
  }
  inline DatumPtr lookupDatum(const std::string& name) const
  {
    DatumTable::const_iterator it=datums_.find(name);
    if (it==datums_.end())
      throw insight::Exception("Could not lookup datum "+name);
    return it->second;
  }
  inline FeaturePtr lookupModelstep(const std::string& name) const
  {
    ModelstepTable::const_iterator it=modelsteps_.find(name);
    if (it==modelsteps_.end())
      throw insight::Exception("Could not lookup model step "+name);
    return it->second;
  }
  inline FeatureSetPtr lookupVertexFeature(const std::string& name) const
  {
    VertexFeatureTable::const_iterator it=vertexFeatures_.find(name);
    if (it==vertexFeatures_.end())
      throw insight::Exception("Could not lookup vertex feature "+name);
    return it->second;
  }
  inline FeatureSetPtr lookupEdgeFeature(const std::string& name) const
  {
    EdgeFeatureTable::const_iterator it=edgeFeatures_.find(name);
    if (it==edgeFeatures_.end())
      throw insight::Exception("Could not lookup edge feature "+name);
    return it->second;
  }
  inline FeatureSetPtr lookupFaceFeature(const std::string& name) const
  {
    FaceFeatureTable::const_iterator it=faceFeatures_.find(name);
    if (it==faceFeatures_.end())
      throw insight::Exception("Could not lookup face feature "+name);
    return it->second;
  }
  inline FeatureSetPtr lookupSolidFeature(const std::string& name) const
  {
    SolidFeatureTable::const_iterator it=solidFeatures_.find(name);
    if (it==solidFeatures_.end())
      throw insight::Exception("Could not lookup solid feature "+name);
    return it->second;
  }
  inline ModelPtr lookupModel(const std::string& name) const
  {
    ModelTable::const_iterator it=models_.find(name);
    if (it==models_.end())
      throw insight::Exception("Could not lookup model "+name);
    return it->second;
  }
  inline PostprocActionPtr lookupPostprocActionSymbol(const std::string& name) const
  {
    PostprocActionTable::const_iterator it=postprocActions_.find(name);
    if (it==postprocActions_.end())
      throw insight::Exception("Could not lookup evaluation "+name);
    return it->second;
  }
  
  const ScalarTable& 		scalars() const 	{ return scalars_; }
  const VectorTable& 		vectors() const 	{ return vectors_; }
  const DatumTable& 		datums() const		{ return datums_; }
  const ModelstepTable& 	modelsteps() const 	{ return modelsteps_; }  
  const VertexFeatureTable& 	vertexFeatures() const 	{ return vertexFeatures_; }  
  const EdgeFeatureTable& 	edgeFeatures() const 	{ return edgeFeatures_; }  
  const FaceFeatureTable& 	faceFeatures() const 	{ return faceFeatures_; }  
  const SolidFeatureTable& 	solidFeatures() const 	{ return solidFeatures_; }  
  const ModelTable& 		models() const 		{ return models_; }  
  const PostprocActionTable& 	postprocActions() const { return postprocActions_; }  

  arma::mat modelCoG();
//   inline arma::mat modelCoG() { return modelCoG(); }; // "const" caused failure with phx::bind!
  
  /**
   * return bounding box of model
   * first col: min point
   * second col: max point
   */
  arma::mat modelBndBox(double deflection=-1) const;

};



}
}

#endif // INSIGHT_CAD_MODEL_H
