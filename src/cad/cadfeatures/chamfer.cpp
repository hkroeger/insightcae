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

#include "chamfer.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(Chamfer);
addToFactoryTable(Feature, Chamfer, NoParameters);




Chamfer::Chamfer(const NoParameters& nop): DerivedFeature(nop)
{}




Chamfer::Chamfer(FeatureSetPtr edges, ScalarPtr l, ScalarPtr angle)
: DerivedFeature(edges->model()), edges_(edges), l_(l), angle_(angle)
{}




FeaturePtr Chamfer::create(FeatureSetPtr edges, ScalarPtr l, ScalarPtr angle)
{
    return FeaturePtr(new Chamfer(edges, l, angle));
}




void Chamfer::build()
{
    double l1=l_->value();
    double l2=::tan(angle_->value())*l1;

    const Feature& m1=*(edges_->model());

    m1.unsetLeaf();
    BRepFilletAPI_MakeChamfer fb(m1);
    BOOST_FOREACH(FeatureID f, edges_->data())
    {
        TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
        TopExp::MapShapesAndAncestors(m1, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
        fb.Add(l1, l2, m1.edge(f), TopoDS::Face(mapEdgeFace(f).First()) );
    }
    fb.Build();
    setShape(fb.Shape());
}




void Chamfer::insertrule(parser::ISCADParser& ruleset) const
{
    ruleset.modelstepFunctionRules.add
    (
        "Chamfer",
        typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(
            ( '(' 
                > ruleset.r_edgeFeaturesExpression > ',' 
                > ruleset.r_scalarExpression 
                > ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst(45.*M_PI/180.)) ) 
                > ')' )
            [ qi::_val = phx::bind(&Chamfer::create, qi::_1, qi::_2, qi::_3) ]

        ))
    );
}



FeatureCmdInfoList Chamfer::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Chamfer",
         
            "( <edgeSelection:edges>, <scalar:l> )",
         
            "Creates chamfers at selected edges of a solid. All edges in the selection set edges are chamfered with width l."
        )
    );
}


}
}
