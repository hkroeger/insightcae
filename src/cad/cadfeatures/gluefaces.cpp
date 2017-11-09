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

#include "gluefaces.h"
#include "occinclude.h"

#include "BRepCheck_Shell.hxx"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "GEOM/GEOMAlgo_Gluer2.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(GlueFaces);
addToFactoryTable(Feature, GlueFaces);

GlueFaces::GlueFaces()
{}


GlueFaces::GlueFaces(FeaturePtr feat, ScalarPtr tol)
: feat_(feat), tol_(tol)
{
    ParameterListHash h(this);
    h+=this->type();
    h+=*feat;
    h+=tol_->value();
}

void GlueFaces::build()
{
  // Example: https://github.com/FedoraScientific/salome-geom/blob/master/src/GEOMImpl/GEOMImpl_GlueDriver.cxx
  GEOMAlgo_Gluer2 ggl;
  
  ggl.SetArgument(feat_->shape());
  ggl.SetTolerance(tol_->value());

  ggl.Detect();
  ggl.Perform();
  
  setShape( ggl.Shape() );
}

void GlueFaces::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "GlueFaces",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression 
	  >> ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst(1e-3)) ) 
      >> ')' )
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<GlueFaces>(qi::_1, qi::_2)) ]
      
    ))
  );
}


}
}

