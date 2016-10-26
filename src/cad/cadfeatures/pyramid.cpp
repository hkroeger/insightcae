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

#include "pyramid.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(Pyramid);
addToFactoryTable(Feature, Pyramid, NoParameters);




Pyramid::Pyramid(const NoParameters& nop)
: Feature(nop)
{}




Pyramid::Pyramid(FeaturePtr base, VectorPtr ptip)
: Feature(),
  base_(base),
  ptip_(ptip)
{
}




FeaturePtr Pyramid::create ( FeaturePtr base, VectorPtr ptip )
{
    return FeaturePtr(new Pyramid(base, ptip));
}




void Pyramid::build()
{
    TopoDS_Shape base=base_->shape();
    gp_Pnt tip=to_Pnt ( ptip_->value() );
    TopoDS_Vertex vtip=BRepBuilderAPI_MakeVertex ( tip );

    BRepBuilderAPI_Sewing sew ( /*tol_->value()*/1e-3 );
    sew.Add ( TopoDS::Face ( base ) );

    for ( TopExp_Explorer ex ( base, TopAbs_EDGE ); ex.More(); ex.Next() ) {
        TopoDS_Edge e=TopoDS::Edge ( ex.Current() );
        TopoDS_Vertex v1=TopExp::FirstVertex ( e );
        TopoDS_Vertex v2=TopExp::LastVertex ( e );

        BRepBuilderAPI_MakeWire mw;
        mw.Add ( e );
        mw.Add ( BRepBuilderAPI_MakeEdge ( v1, vtip ) );
        mw.Add ( BRepBuilderAPI_MakeEdge ( v2, vtip ) );

        TopoDS_Face f=BRepBuilderAPI_MakeFace ( mw.Wire() );
        sew.Add ( f );
    }

    sew.Perform();
    sew.Dump();

    TopoDS_Shell sshell = TopoDS::Shell ( sew.SewedShape() );
//   BRepCheck_Shell acheck(sshell);

    BRepBuilderAPI_MakeSolid solidmaker ( sshell );

    if ( !solidmaker.IsDone() ) {
        throw insight::Exception ( "Creation of solid failed!" );
    }

    setShape ( solidmaker.Solid() );
}




void Pyramid::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Pyramid",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ')' )
      [ qi::_val = phx::bind(&Pyramid::create, qi::_1, qi::_2) ]
      
    ))
  );
}




FeatureCmdInfoList Pyramid::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Pyramid",
            "( <feature:base>, <vector:ptip> )",
            "Creates a pyramid from the planar base feature and the tip point ptip."
        )
    );
}



}
}
