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

#include "wirefillet.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {




defineType(WireFillet);
addToFactoryTable(Feature, WireFillet);


size_t WireFillet::calcHash() const
{
  ParameterListHash h;
  h+=*vertices_;
  h+=r_->value();
  return h.getHash();
}


WireFillet::WireFillet(): DerivedFeature()
{}





WireFillet::WireFillet(FeatureSetPtr vertices, ScalarPtr r)
: DerivedFeature(vertices->model()), vertices_(vertices), r_(r)
{}




FeaturePtr WireFillet::create(FeatureSetPtr vertices, ScalarPtr r)
{
    return FeaturePtr(new WireFillet(vertices, r));
}



void WireFillet::build()
{
    const Feature& m1=* ( vertices_->model() );
    m1.unsetLeaf();

    TopoDS_Wire w = m1.asSingleWire();

    std::vector<TopoDS_Edge> edgs;
    std::vector<gp_Pnt> new_start, new_end;
    for ( BRepTools_WireExplorer wx(w); wx.More(); wx.Next() )
      {
        TopoDS_Edge e=wx.Current();
        edgs.push_back(e);
        new_start.push_back(BRep_Tool::Pnt(TopExp::FirstVertex(e, true)));
        new_end.push_back(BRep_Tool::Pnt(TopExp::LastVertex(e, true)));
      }

    double R=r_->value();

//    TopoDS_Compound res;
//    BRep_Builder b;
//    b.MakeCompound(res);
//    b.Add(res, w);

    TopTools_ListOfShape we;

    for (size_t i=1; i<edgs.size(); i++)
      {
        TopoDS_Edge e1=edgs[i-1];
        TopoDS_Edge e2=edgs[i];
        TopoDS_Vertex v=TopExp::FirstVertex(e2, true);
        gp_Pnt p = BRep_Tool::Pnt(v);

        bool selected=false;
        for ( FeatureID f: vertices_->data() )
          {
            if (m1.vertex(f).IsEqual(v))
              {
                selected=true;
                break;
              }
          }

        if (selected)
          {
            BRepAdaptor_Curve a1(e1);
            BRepAdaptor_Curve a2(e2);
            if ( (a1.GetType()!=GeomAbs_Line) || (a2.GetType()!=GeomAbs_Line) )
              {
                throw insight::Exception("WireFillet: only fillets between lines are currently supported.");
              }

            double u1=a1.LastParameter();
            double u2=a2.FirstParameter();
            gp_Pnt p1, p2; gp_Vec v1, v2;
            a1.D1(u1, p1, v1); v1.Normalize(); v1.Scale(-1);
            a2.D1(u2, p2, v2); v2.Normalize();

            if ( p1.Distance(p)>1e-10 || p2.Distance(p)>1e-10 )
              throw insight::Exception("Internal error: orientation of BRepAdaptor_Curve different than expected!");

            gp_XYZ vert =v1.XYZ().Crossed(v2.XYZ());

            if (vert.Modulus()>0)
              {
                double alphaBy2=0.5*v1.Angle(v2);
                double x=R/std::sin(alphaBy2);
                double d=std::sqrt(x*x-R*R);
                gp_Dir ax( vert );
                gp_XYZ ec=0.5*(v1.XYZ()+v2.XYZ());
                ec.Normalize();

                gp_Pnt p1(p.XYZ()+v1.XYZ()*d);
                gp_Pnt pm(p.XYZ()+ec*(x-R));
                gp_Pnt p2(p.XYZ()+v2.XYZ()*d);
                //gp_Circ c( gp_Ax2( gp_Pnt pm(p.XYZ()+ec*x), ax ), R );

                auto c = GC_MakeArcOfCircle(p1, pm, p2).Value();

                new_end[i-1]=p1;
                new_start[i]=p2;
//                b.Add(res, BRepBuilderAPI_MakeEdge(c));
                we.Append(BRepBuilderAPI_MakeEdge(c));
              }
          }
      }

    for (size_t i=0; i<edgs.size(); i++)
      {
        double u1, u2;
        Handle_Geom_Curve crv = BRep_Tool::Curve(edgs[i], u1, u2);
//        b.Add(res, BRepBuilderAPI_MakeEdge(crv, new_start[i], new_end[i]));
        we.Append(BRepBuilderAPI_MakeEdge(crv, new_start[i], new_end[i]));
      }

    BRepBuilderAPI_MakeWire wb;
    wb.Add ( we );

    ShapeFix_Wire wf;
    wf.Load(wb.Wire());
    wf.Perform();

    setShape ( wf.Wire() );
}



/*! \page Fillet Fillet
  * Create a fillet on an edge.
  *
  * Syntax:
  * ~~~~
  * Fillet(<edge feature set: edges>, <scalar: radius>) : feature
  * ~~~~
  */
void WireFillet::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "WireFillet",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

    ( '(' >> ruleset.r_vertexFeaturesExpression >> ',' >> ruleset.r_scalarExpression >> ')' )
      [ qi::_val = phx::bind(&WireFillet::create, qi::_1, qi::_2) ]

    ))
  );
}




FeatureCmdInfoList WireFillet::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "WireFillet",

            "( <vertexSelection:vertices>, <scalar:r> )",

            "Creates fillets at selected vertices of a wire. All vertices in the selection set vertices are rounded with width r."
        )
    );
}



}
}
