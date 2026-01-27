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

#include "export.h"
#include "cadfeature.h"
#include "datum.h"

namespace insight 
{
namespace cad 
{

defineType(Export);

addToStaticFunctionTable2(
    PostprocAction, InsertRule, insertrule,
    Export, &Export::insertrule );

size_t Export::calcHash() const
{
  ParameterListHash h;
  if (model_) h+=*model_;
  h+=filename_;
#warning extend hash!
  return h.getHash();
}
  
Export::Export
(
  FeaturePtr model, 
  const boost::filesystem::path& filename,
  ExportNamedFeatures namedfeats
)
: model_(model),
  filename_(filename),
  namedfeats_(namedfeats)
{}


void Export::build()
{
   model_->saveAs(filename_, namedfeats_);
}

Handle_AIS_InteractiveObject Export::createAISRepr() const
{
  checkForBuildDuringAccess();
  return Handle_AIS_InteractiveObject();
}

void Export::write(std::ostream& ) const
{}


void Export::insertrule(parser::ISCADParser& rs)
{
    rs.postProcFunctionRules.add
        (
            "saveAs",
            std::make_shared<parser::ISCADParser::PostProcFunctionRule>(
                ( '(' > rs.r_path > ')' > qi::lit("<<")
                 > rs.r_solidmodel_expression
                 > *( rs.r_identifier > '=' > rs.r_faceFeaturesExpression )
                 > ';' )
                    [ qi::_val = phx::bind(
                         &Export::create<FeaturePtr, const boost::filesystem::path&, ExportNamedFeatures>,
                         qi::_2, qi::_1, qi::_3) ]
                )
            );
}



defineType(ExportEMesh);

addToStaticFunctionTable2(
    PostprocAction, InsertRule, insertrule,
    ExportEMesh, &ExportEMesh::insertrule );

size_t ExportEMesh::calcHash() const
{
  ParameterListHash h;
  h+=filename_;
  h+=*eMesh_featureSet_;
  h+=*eMesh_accuracy_;
  h+=*eMesh_accuracy_;
  return h.getHash();
}



ExportEMesh::ExportEMesh(FeatureSetPtr eMesh_featureSet, const boost::filesystem::path& filename, ScalarPtr eMesh_accuracy, ScalarPtr eMesh_maxlen)
: filename_(filename),
  eMesh_featureSet_(eMesh_featureSet),
  eMesh_accuracy_(eMesh_accuracy),
  eMesh_maxlen_(eMesh_maxlen)
{
}

void ExportEMesh::build()
{
  eMesh_featureSet_->model()->exportEMesh(filename_, *eMesh_featureSet_, eMesh_accuracy_->value(), eMesh_maxlen_->value());
}

Handle_AIS_InteractiveObject ExportEMesh::createAISRepr() const
{
  checkForBuildDuringAccess();
  return Handle_AIS_InteractiveObject();
}

void ExportEMesh::write(std::ostream& ) const
{}

void ExportEMesh::insertrule(parser::ISCADParser& rs)
{
    rs.postProcFunctionRules.add
        (
            "exportEMesh",
            std::make_shared<parser::ISCADParser::PostProcFunctionRule>(
                ( '(' > rs.r_path > ',' > rs.r_scalarExpression > ',' > rs.r_scalarExpression > ')'
                 > qi::lit("<<") > rs.r_edgeFeaturesExpression > ';' )
                    [ qi::_val = phx::bind(
                         &ExportEMesh::create
                         <FeatureSetPtr, const boost::filesystem::path&, ScalarPtr, ScalarPtr>,
                         qi::_4, qi::_1, qi::_2, qi::_3) ]
                )
            );
}


defineType(ExportSTL);

addToStaticFunctionTable2(
    PostprocAction, InsertRule, insertrule,
    ExportSTL, &ExportSTL::insertrule );

size_t ExportSTL::calcHash() const
{
  ParameterListHash h;
  h+=*model_;
  h+=filename_;
  if (STL_accuracy_) h+=*STL_accuracy_;
  h+=force_binary_;
  return h.getHash();
}



ExportSTL::ExportSTL(FeaturePtr model, const boost::filesystem::path& filename, ScalarPtr STL_accuracy, bool force_binary)
: model_(model),
  filename_(filename),
  STL_accuracy_(STL_accuracy),
  force_binary_(force_binary)
{
}


void ExportSTL::build()
{

  double abstol=5e-5;
  if (STL_accuracy_)
    {
      abstol=*STL_accuracy_;
    }

  bool binary=false;
  std::string ext=filename_.extension().string();
  if (force_binary_)
    {
      binary=true;
    }
  else
    {
      if (ext==".stlb") binary=true;
    }

  model_->exportSTL(filename_, abstol, binary);
}

Handle_AIS_InteractiveObject ExportSTL::createAISRepr() const
{
  checkForBuildDuringAccess();
  return Handle_AIS_InteractiveObject();
}

void ExportSTL::write(std::ostream& ) const
{}


void ExportSTL::insertrule(parser::ISCADParser& rs)
{
    rs.postProcFunctionRules.add
        (
            "exportSTL",
            std::make_shared<parser::ISCADParser::PostProcFunctionRule>(
                ( '('
                 > rs.r_path
                 > ( (',' > rs.r_scalarExpression)|qi::attr(ScalarPtr()) )
                 > ( (',' > qi::lit("ascii") > qi::attr(false) )|qi::attr(true) )
                 > ')' > qi::lit("<<") > rs.r_solidmodel_expression > ';' )
                    [ qi::_val = phx::bind(
                        &ExportSTL::create<FeaturePtr, const boost::filesystem::path&, ScalarPtr, bool>,
                        qi::_4, qi::_1, qi::_2, qi::_3) ]
                )
            );
}


}
}
