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

#include "arc.h"
#include "base/boost_include.h"
#include "base/translations.h"

#include <boost/spirit/include/qi.hpp>

#ifndef Q_MOC_RUN
#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>
#endif
#include <boost/spirit/home/support/iterators/line_pos_iterator.hpp>
#include <boost/spirit/repository/include/qi_iter_pos.hpp>


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
  
    
    
defineType(Arc);
//addToFactoryTable(Feature, Arc);
addToStaticFunctionTable(Feature, Arc, insertrule);
addToStaticFunctionTable(Feature, Arc, ruleDocumentation);


size_t Arc::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=p0_->value();
  h+=p0tang_->value();
  h+=p1_->value();
  return h.getHash();
}





void Arc::build()
{
  Handle_Geom_TrimmedCurve crv =
      GC_MakeArcOfCircle(
      to_Pnt(*p0_),
      to_Vec(*p0tang_),
      to_Pnt(*p1_) );

  setShape(BRepBuilderAPI_MakeEdge(crv));
  
  auto c = Handle_Geom_Circle::DownCast(crv->BasisCurve())->Circ();
  refpoints_["center"]=vec3(c.Location());
  refvalues_["D"]=c.Radius()*2.;
  refvectors_["normal"]=vec3(c.Axis().Direction());
}




Arc::Arc(VectorPtr p0, VectorPtr p0tang, VectorPtr p1)
: p0_(p0), p0tang_(p0tang), p1_(p1)
{}






/**
 * \page iscad_arc Arc
 *
 * The "Arc" command creates an arc curve. The arc is specified by its start point "p0" and end point "p1" together with its tangent direction "tan" at the start point.
 * 
 * \section syntax Syntax
 * 
 * <b> Arc(\ref iscad_vector_expression "<vector:p0>", \ref iscad_vector_expression "<vector:tan> ", \ref iscad_vector_expression "<vector:p1>") </b>
 * 
 * \section return Return Value
 * 
 * \ref iscad_feature_expression "Feature"
 * 
 * \section provides Provided Properties
 * 
 * Points:
 * * "p0": start point
 * * "p1": end point
 * 
 * Vectors:
 * * "et0": tangent vector at p0
 * * "et1": tangent vector at p1
 */


void Arc::insertrule(parser::ISCADParser& ruleset)
{
  using boost::spirit::repository::qi::iter_pos;

  ruleset.modelstepFunctionRules.add
  (
    "Arc",
          std::make_shared<parser::ISCADParser::ModelstepRule>(

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' )
                  [ qi::_val = phx::bind(&Arc::create<VectorPtr, VectorPtr, VectorPtr>, qi::_1, qi::_2, qi::_3) ]

    )
  );
}




FeatureCmdInfoList Arc::ruleDocumentation()
{
  return {
        FeatureCmdInfo
        (
            "Arc",
            "( <vector:p0>, <vector:et0>, <vector:p1> )",
          _("Creates an arc between point p0 and p1. At point p0, the arc is tangent to vector et0.")
        )
  };
}







bool Arc::isSingleOpenWire() const
{
  return true;
}




defineType(Arc3P);
//addToFactoryTable(Feature, Arc3P);
addToStaticFunctionTable(Feature, Arc3P, insertrule);
addToStaticFunctionTable(Feature, Arc3P, ruleDocumentation);


size_t Arc3P::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=p0_->value();
  h+=pm_->value();
  h+=p1_->value();
  return h.getHash();
}





void Arc3P::build()
{
  Handle_Geom_TrimmedCurve crv=GC_MakeArcOfCircle(to_Pnt(*p0_), to_Pnt(*p1_), to_Pnt(*pm_));
  
  setShape(BRepBuilderAPI_MakeEdge(crv));

  auto c = Handle_Geom_Circle::DownCast(crv->BasisCurve())->Circ();
  refpoints_["center"]=vec3(c.Location());
  refvalues_["D"]=c.Radius()*2.;
  refvectors_["normal"]=vec3(c.Axis().Direction());
}




Arc3P::Arc3P(VectorPtr p0, VectorPtr pm, VectorPtr p1)
: p0_(p0), pm_(pm), p1_(p1)
{}






void Arc3P::insertrule(parser::ISCADParser& ruleset)
{
  using boost::spirit::repository::qi::iter_pos;
  
  ruleset.modelstepFunctionRules.add
  (
    "Arc3P",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ')' )
                  [ qi::_val = phx::bind(&Arc3P::create<VectorPtr, VectorPtr, VectorPtr>, qi::_1, qi::_2, qi::_3) ]
      
    ))
  );
}




FeatureCmdInfoList Arc3P::ruleDocumentation()
{
  return {
        FeatureCmdInfo
        (
            "Arc3P",
            "( <vector:p0>, <vector:pm>, <vector:p1> )",
          _("Creates an arc between point p0 and p1 through intermediate point pm.")
        )
  };
}





bool Arc3P::isSingleOpenWire() const
{
  return true;
}




}
}
