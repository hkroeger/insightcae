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
#include "base/translations.h"
#include "BRepOffsetAPI_MakeFilling.hxx"

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

  if (const auto* e1 = boost::get<FeaturePtr>(&e1_))
  {
      h+=*(*e1);
  }
  else if (const auto* es1 = boost::get<FeatureSetPtr>(&e1_))
  {
      h+=*(*es1);
  }

  if (const auto* e2 = boost::get<FeaturePtr>(&e2_))
  {
      h+=*(*e2);
  }
  else if (const auto* es2 = boost::get<FeatureSetPtr>(&e2_))
  {
      h+=*(*es2);
  }

  h+=inverted_;

  return h.getHash();
}





FillingFace::FillingFace ( EdgeInput e1, EdgeInput e2, bool inverted )
    : e1_ ( e1 ), e2_ ( e2 ), inverted_(inverted)
{
}





void FillingFace::build()
{
  TopoDS_Edge ee1, ee2;

  bool ok=true;
  if (const auto* e1 = boost::get<FeaturePtr>(&e1_))
  {
      if ( (*e1)->isSingleEdge() )
      {
          ee1=(*e1)->asSingleEdge();
      }
      else
      {
          throw insight::Exception ( _("first edge feature is not a single edge!") );
      }
  }
  else if (const auto* es1 = boost::get<FeatureSetPtr>(&e1_))
  {
      if ( (*es1)->size() !=1 )
      {
          throw insight::Exception ( _("first feature set has to contain only 1 edge!") );
      }
      else
      {
          ee1=(*es1)->model()->edge ( *(*es1)->data().begin() );
      }
  }
  else
  {
      throw insight::Exception ( "internal error" );
  }

  if (const auto* e2 = boost::get<FeaturePtr>(&e2_))
  {
      if ( (*e2)->isSingleEdge() )
      {
          ee2=(*e2)->asSingleEdge();
      }
      else
      {
          throw insight::Exception ( _("second edge feature is not a single edge!") );
      }
  }
  else if (const auto* es2 = boost::get<FeatureSetPtr>(&e2_))
  {
      if ( (*es2)->size() !=1 )
      {
          throw insight::Exception ( _("second feature set has to contain only 1 edge!") );
      }
      else
      {
          ee2=(*es2)->model()->edge ( *(*es2)->data().begin() );
      }
  }
  else
  {
      throw insight::Exception ( "internal error" );
  }

  if (inverted_)
  {
      ee2.Reverse();
  }

  TopoDS_Face f;
  try
  {
//      f=BRepFill::Face ( ee1, ee2 );
      BRepOffsetAPI_MakeFilling fsb;
      fsb.Add(ee1, GeomAbs_C0);
      fsb.Add(BRepBuilderAPI_MakeEdge(TopExp::LastVertex(ee1), TopExp::LastVertex(ee2)), GeomAbs_C0);
      fsb.Add(ee2, GeomAbs_C0);
      fsb.Add(BRepBuilderAPI_MakeEdge(TopExp::FirstVertex(ee2), TopExp::FirstVertex(ee1)), GeomAbs_C0);
      fsb.Build();
      f=TopoDS::Face(fsb.Shape());
  }
  catch ( ... )
  {
      throw insight::Exception ( _("Failed to generate face!") );
  }

//  setShape(f);

  ShapeFix_Face FixShape;
  FixShape.Init ( f );
  FixShape.Perform();

  setShape ( FixShape.Face() );


  providedSubshapes_.insert(
      {
          "tan1",
          cad::ImportedSingleEdgeFeature::create(ee1)
      });
  providedSubshapes_.insert(
      {
          "tan2",
          cad::ImportedSingleEdgeFeature::create(ee2)
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
           ( '('
             > (ruleset.r_edgeFeaturesExpression|ruleset.r_solidmodel_expression) // FS first, because might start with solidmodel expr
             > ','
             > (ruleset.r_edgeFeaturesExpression|ruleset.r_solidmodel_expression)
             > ( ( ',' > qi::lit("inverted") > qi::attr(true) | qi::attr(false) ) )
             > ')' )
            [ qi::_val = phx::bind(
                    &FillingFace::create<EdgeInput, EdgeInput, bool>,
                     qi::_1, qi::_2, qi::_3 ) ]
            )
    );
}




FeatureCmdInfoList FillingFace::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "FillingFace",
         
            "( <feature:e0>|<edgeSelection:e0>, <feature:e1>|<edgeSelection:e1 [, inverted] )",

            _("Creates an interpolated surface between two edges. The two edges e0 and e1 can be given either as edge features or edge selection sets.")
        )
    };
}



}
}
