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

#include "fillingface.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "featurefilters/edgeconnectingvertices.h"
#include "cadfeatures/singleedgefeature.h"


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    

defineType(FillingFace);
//addToFactoryTable(Feature, FillingFace);
addToStaticFunctionTable(Feature, FillingFace, insertrule);
addToStaticFunctionTable(Feature, FillingFace, ruleDocumentation);



size_t FillingFace::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  if (e1_ && e2_)
    {
      h+=*e1_;
      h+=*e2_;
    }
  else
    {
      h+=*es1_;
      h+=*es2_;
    }
  return h.getHash();
}





FillingFace::FillingFace ( FeaturePtr e1, FeaturePtr e2 )
    : e1_ ( e1 ), e2_ ( e2 )
{
}




FillingFace::FillingFace ( FeatureSetPtr es1, FeatureSetPtr es2 )
    : es1_ ( es1 ), es2_ ( es2 )
{}





void FillingFace::build()
{
  TopoDS_Edge ee1, ee2;

  if ( e1_ && e2_ )
  {
      bool ok=true;
      if ( e1_->isSingleEdge() )
      {
          ee1=e1_->asSingleEdge();
      }
      else
      {
          ok=false;
      }
      if ( e2_->isSingleEdge() )
      {
          ee2=e2_->asSingleEdge();
      }
      else
      {
          ok=false;
      }

      if ( !ok )
      {
          throw insight::Exception ( "Invalid edge given!" );
      }
  }
  else if ( es1_ && es2_ )
  {
      TopoDS_Edge ee1, ee2;
      if ( es1_->size() !=1 )
      {
          throw insight::Exception ( "first feature set has to contain only 1 edge!" );
      }
      else
      {
          ee1=es1_->model()->edge ( *es1_->data().begin() );
      }

      if ( es2_->size() !=1 )
      {
          throw insight::Exception ( "second feature set has to contain only 1 edge!" );
      }
      else
      {
          ee2=es2_->model()->edge ( *es2_->data().begin() );
      }
  }
  else
  {
      throw insight::Exception ( "Improper specification of edges for FillingFace!" );
  }

  TopoDS_Face f;
  try
  {
      f=BRepFill::Face ( ee1, ee2 );
  }
  catch ( ... )
  {
      throw insight::Exception ( "Failed to generate face!" );
  }

  ShapeFix_Face FixShape;
  FixShape.Init ( f );
  FixShape.Perform();

  setShape ( FixShape.Face() );

  auto va1 = vertexID(TopExp::FirstVertex(ee1));
  auto vb1 = vertexID(TopExp::LastVertex(ee1));
  auto va2 = vertexID(TopExp::FirstVertex(ee2));
  auto vb2 = vertexID(TopExp::LastVertex(ee2));

  providedSubshapes_.insert(
      {
          "tan1",
          cad::SingleEdgeFeature::create(
              std::make_shared<FeatureSet>(
                  shared_from_this(),
                  EntityType::Edge,
                  query_edges(
                      std::make_shared<EdgeConnectingVertices>(
                          va1, va2))))
      });
  providedSubshapes_.insert(
      {
          "tan2",
          cad::SingleEdgeFeature::create(
              std::make_shared<FeatureSet>(
                  shared_from_this(),
                  EntityType::Edge,
                  query_edges(
                      std::make_shared<EdgeConnectingVertices>(
                          vb1, vb2))))
      });
}




FillingFace::operator const TopoDS_Face& () const
{
    return TopoDS::Face ( shape() );
}




void FillingFace::insertrule ( parser::ISCADParser& ruleset )
{
    ruleset.modelstepFunctionRules.add
    (
        "FillingFace",
        std::make_shared<parser::ISCADParser::ModelstepRule>(

                    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_solidmodel_expression >> ')' )
                    [ qi::_val = phx::bind(
                         &FillingFace::create<FeaturePtr, FeaturePtr>,
                         qi::_1, qi::_2 ) ]
                    |
                    ( '(' >> ruleset.r_edgeFeaturesExpression >> ',' >> ruleset.r_edgeFeaturesExpression >> ')' )
                    [ qi::_val = phx::bind(
                         &FillingFace::create<FeatureSetPtr, FeatureSetPtr>,
                         qi::_1, qi::_2 ) ]

                )
    );
}




FeatureCmdInfoList FillingFace::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "FillingFace",
         
            "( (<feature:e0>, <feature:e1>) | (<edgeSelection:e0>, <edgeSelection:e1) )",
         
            "Creates an interpolated surface between two edges. The two edges e0 and e1 can be given either as edge features or edge selection sets."
        )
    };
}



}
}
