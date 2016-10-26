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

#include "sweep.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;




namespace insight {
namespace cad {

    
    
    
defineType(Sweep);
addToFactoryTable(Feature, Sweep, NoParameters);




Sweep::Sweep(const NoParameters& nop): Feature(nop)
{}




Sweep::Sweep(const std::vector<FeaturePtr>& secs)
: secs_(secs)
{}




FeaturePtr Sweep::create ( const std::vector<FeaturePtr>& secs )
{
    return FeaturePtr(new Sweep(secs));
}




void Sweep::build()
{
    if ( secs_.size() <2 ) {
        throw insight::Exception ( "Insufficient number of sections given!" );
    }

    bool create_solid=false;
    {
        TopoDS_Shape cs0=*secs_[0];
        if ( cs0.ShapeType() ==TopAbs_FACE ) {
            create_solid=true;
        } else if ( cs0.ShapeType() ==TopAbs_WIRE ) {
            create_solid=TopoDS::Wire ( cs0 ).Closed();
        }
    }

    BRepOffsetAPI_ThruSections sb ( create_solid );

    BOOST_FOREACH ( const FeaturePtr& skp, secs_ ) {
        TopoDS_Wire cursec;
        TopoDS_Shape cs=*skp;
        if ( cs.ShapeType() ==TopAbs_FACE ) {
            cursec=BRepTools::OuterWire ( TopoDS::Face ( cs ) );
        } else if ( cs.ShapeType() ==TopAbs_WIRE ) {
            cursec=TopoDS::Wire ( cs );
        } else if ( cs.ShapeType() ==TopAbs_EDGE ) {
            BRepBuilderAPI_MakeWire w;
            w.Add ( TopoDS::Edge ( cs ) );
            cursec=w.Wire();
        }
//     else if (cs.ShapeType()==TopAbs_SHELL)
//     {
//      cursec=BRepTools::OuterWire(TopoDS::Shell(cs));
//     }
        else {
            throw insight::Exception ( "Incompatible section shape for Sweep!" );
        }
        sb.AddWire ( cursec );
    }

    setShape ( sb.Shape() );
}




void Sweep::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Sweep",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> (ruleset.r_solidmodel_expression % ',' ) >> ')' ) 
      [ qi::_val = phx::bind(&Sweep::create, qi::_1) ]
      
    ))
  );
}




FeatureCmdInfoList Sweep::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Sweep",
            "( <feature:xsec0>, ..., <feature:xsecn> )",
            "Interpolates a solid through the planar sections xsec0 to xsecn."
        )
    );
}



}
}
