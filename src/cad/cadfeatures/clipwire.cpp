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


#include "GCPnts_AbscissaPoint.hxx"

#include "clipwire.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    

defineType(ClipWire);
addToFactoryTable(Feature, ClipWire, NoParameters);




ClipWire::ClipWire(const NoParameters&)
{
}




ClipWire::ClipWire(FeaturePtr wire, ScalarPtr ls, ScalarPtr le)
: DerivedFeature(wire),
  m1_(wire), ls_(ls), le_(le)
{
}




FeaturePtr ClipWire::create(FeaturePtr wire, ScalarPtr ls, ScalarPtr le)
{
    return FeaturePtr(new ClipWire(wire, ls, le));
}




void ClipWire::build()
{
    if (!m1_->isSingleOpenWire())
    {
        throw insight::Exception("Given feature is not a wire! ClipWire can only operate on wires.");
    }

    TopoDS_Wire w = m1_->asSingleOpenWire();

    double Ls=ls_->value();
    double Le=le_->value();

    BRepAdaptor_CompCurve crv(w);
    double u0=crv.FirstParameter(), u1=crv.LastParameter();
    gp_Pnt ps, pe;
    {
        CPnts_AbscissaPoint ap(crv, Ls, u0, 0.5*(u0+u1), 1e-6);
        ps=crv.Value(ap.Parameter());
    }
    {
        CPnts_AbscissaPoint ap(crv, -Le, u1, 0.5*(u0+u1), 1e-6);
        pe=crv.Value(ap.Parameter());
    }

    int i0=-1, i1=-1;
    std::vector<double> L, L_at_end_from_start, L_at_start_from_end;
    std::vector<TopoDS_Edge> oedgs;
    int j=0;
    for (BRepTools_WireExplorer ex(w); ex.More(); ex.Next())
    {
        TopoDS_Edge e=ex.Current();

        double l=0;
        GProp_GProps gpr;
        if (!BRep_Tool::Degenerated(e))
        {
            BRepGProp::LinearProperties(e, gpr);
            l = gpr.Mass();
        }
        double lastL=0.;
        if (j>0) lastL=L_at_end_from_start.back();

        oedgs.push_back(e);
        L.push_back(l);
        L_at_start_from_end.push_back(lastL);
        L_at_end_from_start.push_back(lastL+l);

        j++;
    }
    size_t n=L_at_end_from_start.size();

    double Ltotal=L_at_end_from_start.back();

    for (size_t i=0; i<n; i++)
    {
        L_at_start_from_end[i] = Ltotal - L_at_start_from_end[i];
    }
    for (int i=0; i<n; i++)
    {
        if (L_at_end_from_start[i]>Ls)
        {
            i0=i;
            break;
        }
    }
    for (int i=n-1; i>=0; i--)
    {
        if (L_at_start_from_end[i]>Le)
        {
            i1=i;
            break;
        }
    }

    TopTools_ListOfShape edgs;
    double t0, t1;
    edgs.Append
    (
        BRepBuilderAPI_MakeEdge
        (
            BRep_Tool::Curve(oedgs[i0], t0, t1),
            ps,
            BRep_Tool::Pnt(TopExp::LastVertex(oedgs[i0], Standard_True))
        )
    );
    for (int i=i0+1; i<=i1-1; i++)
    {
        edgs.Append(oedgs[i]);
    }
    edgs.Append
    (
        BRepBuilderAPI_MakeEdge
        (
            BRep_Tool::Curve(oedgs[i1], t0, t1),
            BRep_Tool::Pnt(TopExp::FirstVertex(oedgs[i1], Standard_True)),
            pe
        )
    );

    refpoints_["p0"]=insight::Vector(ps);
    refpoints_["p1"]=insight::Vector(pe);

    BRepBuilderAPI_MakeWire wb;
    wb.Add(edgs);

    ShapeFix_Wire wf;
    wf.Load(wb.Wire());
    wf.Perform();

    setShape(wf.Wire());
}




void ClipWire::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "ClipWire",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_scalarExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	[ qi::_val = phx::bind(&ClipWire::create, qi::_1, qi::_2, qi::_3) ]
      
    ))
  );
}



FeatureCmdInfoList ClipWire::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "ClipWire",
         
            "( <feature:wire>, <scalar:Ls>, <scalar:Le> )",
         
            "Modifies an open wire feature by clipping its ends. From the beginning, a segment of length Ls is removed and from the end a segment of length Le."
        )
    );
}


}
}
