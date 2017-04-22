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

#include "ellipse.h"
#include "base/boost_include.h"
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

#include "GC_MakeEllipse.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
  
    
    
defineType(Ellipse);
addToFactoryTable(Feature, Ellipse);



Ellipse::Ellipse()
: Feature()
{
}




void Ellipse::build()
{
  arma::mat axmaj=axmaj_->value();
  arma::mat axmin=axmin_->value();

  gp_Ax2 ecs ( to_Pnt ( *p0_ ), to_Vec ( arma::cross ( axmaj, axmin ) ), to_Vec ( axmaj ) );
  Handle_Geom_Ellipse crv=GC_MakeEllipse ( gp_Elips(ecs, arma::norm ( axmaj, 2 ), arma::norm ( axmin,2 )) );
  setShape ( BRepBuilderAPI_MakeEdge ( crv ) );
  
//   gp_Pnt p;
//   gp_Vec v;
//   crv->D1(crv->FirstParameter(), p, v);
//   refpoints_["p0"]=vec3(p);
//   refvectors_["et0"]=vec3(v);
//   crv->D1(crv->LastParameter(), p, v);
//   refpoints_["p1"]=vec3(p);
//   refvectors_["et1"]=vec3(v);
}




Ellipse::Ellipse(VectorPtr p0, VectorPtr axmaj, VectorPtr axmin)
: p0_(p0), axmaj_(axmaj), axmin_(axmin)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=p0_->value();
  h+=axmaj_->value();
  h+=axmin_->value();
}




FeaturePtr Ellipse::create(VectorPtr p0, VectorPtr axmaj, VectorPtr axmin)
{
    return FeaturePtr(new Ellipse(p0, axmaj, axmin));
}




void Ellipse::insertrule(parser::ISCADParser& ruleset) const
{
  using boost::spirit::repository::qi::iter_pos;
  
  ruleset.modelstepFunctionRules.add
  (
    "Ellipse",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' ) 
	 [ qi::_val = phx::bind(&Ellipse::create, qi::_1, qi::_2, qi::_3) ]
      
    ))
  );
}




FeatureCmdInfoList Ellipse::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Ellipse",
            "( <vector:p0>, <vector:axmaj>, <vector:axmin> )",
            "Creates an ellipse around point p0. The major axis has length and direction of axmaj. The minor axis length is that of axmin. "
            "Its direction is corrected to be orthogonal to axmaj."
        )
    );
}




bool Ellipse::isSingleCloseWire() const
{
  return false;
}




bool Ellipse::isSingleOpenWire() const
{
  return true;
}






}
}
