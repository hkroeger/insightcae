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

#include "BRepAdaptor_HCompCurve.hxx"
#include "Approx_Curve3d.hxx"
#include "pipe.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

  
    

defineType(Pipe);
addToFactoryTable(Feature, Pipe, NoParameters);




Pipe::Pipe(const NoParameters& nop): Feature(nop)
{}




Pipe::Pipe(FeaturePtr spine, FeaturePtr xsec, VectorPtr fixed_binormal, bool orient, bool reapprox_spine)
    : spine_(spine), xsec_(xsec), orient_(orient), reapprox_spine_(reapprox_spine), fixed_binormal_(fixed_binormal)
{}



FeaturePtr Pipe::create(FeaturePtr spine, FeaturePtr xsec, VectorPtr fixed_binormal, bool orient, bool reapprox_spine)
{
    return FeaturePtr(new Pipe(spine, xsec, fixed_binormal, orient, reapprox_spine));
}



void Pipe::build()
{
    if (!spine_->isSingleWire())
        throw insight::Exception("spine feature has to provide a singly connected wire!");  // not working for wires created from feature edge selection

    if (!xsec_->isSingleFace() || xsec_->isSingleWire() || xsec_->isSingleEdge())
        throw insight::Exception("xsec feature has to provide a face or wire!");

    TopoDS_Wire spinew=spine_->asSingleWire();

    if (reapprox_spine_)
    {
        BRepAdaptor_CompCurve wireAdaptor(spinew);
        Handle(BRepAdaptor_HCompCurve) curve = new BRepAdaptor_HCompCurve(wireAdaptor);
        Approx_Curve3d approx2(curve, 0.001, GeomAbs_G1, 2000, 6);
        if (approx2.IsDone() && approx2.HasResult())
        {
            Handle_Geom_Curve approxcrv=approx2.Curve();
            spinew=BRepBuilderAPI_MakeWire( BRepBuilderAPI_MakeEdge(approxcrv).Edge() ).Wire();
        }
        else
        {
            throw insight::Exception("Pipe: reapproximation of spine failed!");
        }
    }

    BRepAdaptor_CompCurve w(spinew);
    double p0=w.FirstParameter();
    double p1=w.LastParameter();


    TopoDS_Shape xsec=BRepTools::OuterWire(TopoDS::Face(xsec_->shape()));

    gp_Trsf tr;

    if (!orient_)
        tr.SetTranslation(w.Value(p0).XYZ());
    else
    {
        gp_Pnt v0;
        gp_Vec vp0;
        w.D1(p0, v0, vp0);
        vp0.Normalize();

        gp_Trsf tr1;
        tr1.SetTransformation
        (
            gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(1,0,0)),
            gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(vp0))
        );
        xsec=BRepBuilderAPI_Transform(static_cast<TopoDS_Shape>(xsec), tr1).Shape();

        tr.SetTranslationPart(v0.XYZ());
    }
    TopoDS_Shape xsecs=BRepBuilderAPI_Transform(static_cast<TopoDS_Shape>(xsec), tr).Shape();

    BRepOffsetAPI_MakePipeShell p(spinew);
    p.Add(xsecs);
    
    if (fixed_binormal_)
    {
        p.SetMode(gp_Dir(to_Vec(fixed_binormal_->value())));
    }
    
    p.Build();
    p.MakeSolid();
    
    providedSubshapes_["frontFace"]=FeaturePtr(new Feature(p.FirstShape()));
    providedSubshapes_["backFace"]=FeaturePtr(new Feature(p.LastShape()));
    
    setShape(p.Shape());
}




void Pipe::insertrule(parser::ISCADParser& ruleset) const
{
    ruleset.modelstepFunctionRules.add
    (
        "Pipe",
        typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

                    ( '(' >> ruleset.r_solidmodel_expression >> ','
                      >> ruleset.r_solidmodel_expression
                      >> ( ( ',' >> qi::lit("fixedbinormal") >> ruleset.r_vectorExpression ) | qi::attr(VectorPtr()) ) 
                      >> ( ( ',' >> qi::lit("orient") >> qi::attr(true) ) | qi::attr(false) ) 
                      >> ( ( ',' >> qi::lit("reapprox") >> qi::attr(true) ) | qi::attr(false) ) 
                      >> ')' )
                    [ qi::_val = phx::bind(&Pipe::create, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5) ]

                ))
    );
}




FeatureCmdInfoList Pipe::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Pipe",
            "( <feature:xsec>, <feature:spine> [, fixedbinormal <vector>] [, orient] [, reapprox] )",
            "Sweeps the planar section xsec along the curve feature spine."
            " The xsec is expected at global origin [0,0,0] and is moved to the beginning of the spine."
            " By fixing the binormal direction using keyword fixedbinormal and a fixed direction, problems with erratic twisting of the section on spiral paths can be avoided."
            " By default, the section is not rotated. If keyword reorient is given, the z-axis of the section is aligned with the tangent of the spine."
            " The keyword reapprox triggers an reapproximation of the spine wire into a single b-spline curve (experimental)."
        )
    );
}




}
}
