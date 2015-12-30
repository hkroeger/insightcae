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
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(FillingFace);
addToFactoryTable(SolidModel, FillingFace, NoParameters);


FillingFace::FillingFace(const NoParameters&)
{}

FillingFace::FillingFace(const SolidModel& e1, const SolidModel& e2)
{  
  TopoDS_Edge ee1, ee2;
  bool ok=true;
  if (e1.isSingleEdge())
  {
    ee1=e1.asSingleEdge();
  }
  else ok=false;
  if (e2.isSingleEdge())
  {
    ee2=e2.asSingleEdge();
  }
  else ok=false;

  if (!ok)
    throw insight::Exception("Invalid edge given!");

  TopoDS_Face f;
  try
  {
    f=BRepFill::Face(ee1, ee2);
  }
  catch (...)
  {
    throw insight::Exception("Failed to generate face!");
  }
  
  ShapeFix_Face FixShape;
  FixShape.Init(f);
  FixShape.Perform();
  
  setShape(FixShape.Face());
}

FillingFace::FillingFace(const FeatureSet& e1, const FeatureSet& e2)
{
  /*
  TopoDS_Edge ee1, ee2;
  {
    TopTools_ListOfShape edgs;
    BOOST_FOREACH(const FeatureID& i, e1)
    {
      edgs.Append( e1.model().edge(*e1.begin()) );
    }
    BRepBuilderAPI_MakeWire w;
    w.Add(edgs);
    Handle_Geom_Curve crv(new BRepAdaptor_CompCurve(w.Wire()));
    ee1=BRepBuilderAPI_MakeEdge(crv);
  }

  {
    TopTools_ListOfShape edgs;
    BOOST_FOREACH(const FeatureID& i, e1)
    {
      edgs.Append( e2.model().edge(*e2.begin()) );
    }
    BRepBuilderAPI_MakeWire w;
    w.Add(edgs);
    Handle_Geom_Curve crv(new BRepAdaptor_CompCurve(w.Wire()));
    ee2=BRepBuilderAPI_MakeEdge(crv);
  }
  */
  
  TopoDS_Edge ee1, ee2;
  if (e1.size()!=1)
  {
    throw insight::Exception("first feature set has to contain only 1 edge!");
  }
  else
  {
    ee1=e1.model().edge(*e1.begin());
  }
  
  if (e2.size()!=1)
  {
    throw insight::Exception("second feature set has to contain only 1 edge!");
  }
  else
  {
    ee2=e2.model().edge(*e2.begin());
  }

  TopoDS_Face f;
  try
  {
    f=BRepFill::Face(ee1, ee2);
  }
  catch (...)
  {
    throw insight::Exception("Failed to generate face!");
  }
  
  ShapeFix_Face FixShape;
  FixShape.Init(f);
  FixShape.Perform();
  
  setShape(FixShape.Face());
}

FillingFace::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape_);
}


void FillingFace::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "FillingFace",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_solidmodel_expression >> ')' )
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<FillingFace>(*qi::_1, *qi::_2)) ]
    |
    ( '(' >> ruleset.r_edgeFeaturesExpression >> ',' >> ruleset.r_edgeFeaturesExpression >> ')' )
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<FillingFace>(*qi::_1, *qi::_2)) ]
      
    ))
  );
}

}
}
