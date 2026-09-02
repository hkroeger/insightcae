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

#include "splinecurve.h"
#include "cadfeature.h"
#include "datum.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix/fusion.hpp>
#include "base/translations.h"

#include "TColgp_HArray1OfPnt.hxx"
#include "GeomAPI_Interpolate.hxx"
#include "BRepBuilderAPI_MakePolygon.hxx"
#include "cadfeatures/importsolidmodel.h"


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {

    
    
    
defineType(SplineCurve);
//addToFactoryTable(Feature, SplineCurve);
addToStaticFunctionTable(Feature, SplineCurve, insertrule);
addToStaticFunctionTable(Feature, SplineCurve, ruleDocumentation);


size_t SplineCurve::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  for (const VectorPtr& p: pts_)
  {
      h+=*p;
  }
  if (tan0_) h+=*tan0_;
  if (tan1_) h+=*tan1_;
  return h.getHash();
}




SplineCurve::SplineCurve(const SplineCurve&o, TreeCloneMap& tcm)
    : CL(tan0_), CL(tan1_)
{
    for (auto& p: o.pts_)
    {
        pts_.push_back(tcm.clone(p));
    }
}



SplineCurve::SplineCurve(const std::vector<VectorPtr>& pts, VectorPtr tan0, VectorPtr tan1)
: pts_(pts), tan0_(tan0), tan1_(tan1)
{}







void SplineCurve::build()
{
//     TColgp_Array1OfPnt pts_col ( 1, pts_.size() );
    Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt( 1, pts_.size() );
    for ( int j=0; j<pts_.size(); j++ ) {
        arma::mat pi=pts_[j]->value();
        pts_col->SetValue ( j+1, to_Pnt ( pi ) );
        refpoints_[str(format("p%02d")%j)]=pi;
    }
//     GeomAPI_PointsToBSpline splbuilder ( pts_col );
    GeomAPI_Interpolate splbuilder ( pts_col, false, 1e-6 );
    if (tan0_ && tan1_)
    {
        splbuilder.Load(to_Vec(tan0_->value()), to_Vec(tan1_->value()));
    }
    splbuilder.Perform();
    Handle_Geom_BSplineCurve crv=splbuilder.Curve();
    setShape ( BRepBuilderAPI_MakeEdge ( crv, crv->FirstParameter(), crv->LastParameter() ) );
}




void SplineCurve::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "SplineCurve",	
    std::make_shared<parser::ISCADParser::ModelstepRule>(

    ( '(' 
        > ruleset.r_vectorExpression % ','
        > ( (',' > qi::lit("der") > ruleset.r_vectorExpression > ruleset.r_vectorExpression ) | ( qi::attr(VectorPtr()) >> qi::attr(VectorPtr()) ) )
        > ')' )
    [ qi::_val = phx::bind(
                         &SplineCurve::create<const std::vector<VectorPtr>&, VectorPtr, VectorPtr>,
                         qi::_1, phx::at_c<0>(qi::_2), phx::at_c<1>(qi::_2) ) ]
      
    )
  );
}




FeatureCmdInfoList SplineCurve::ruleDocumentation()
{
  return {
        FeatureCmdInfo
        (
            "SplineCurve",
            "( <vector:p0>, ..., <vector:pn> )",
          _("Creates a spline curve through all given points p0 to pn.")
        )
  };
}

VectorPtr SplineCurve::start() const
{
  return pts_.front();
}

VectorPtr SplineCurve::end() const
{
  return pts_.back();
}




defineType(RapproximatedCurve);
addToStaticFunctionTable(Feature, RapproximatedCurve, insertrule);
addToStaticFunctionTable(Feature, RapproximatedCurve, ruleDocumentation);


size_t RapproximatedCurve::calcHash() const
{
    ParameterListHash h;
    h += this->type();
    h += *p0_;
    h += *p1_;
    h += *orgCurve_;
    if (tan0_) h += *tan0_;
    if (tan1_) h += *tan1_;
    return h.getHash();
}


RapproximatedCurve::RapproximatedCurve(const RapproximatedCurve& o, TreeCloneMap& tcm)
    : CL(p0_), CL(p1_), CL(orgCurve_), CL(tan0_), CL(tan1_)
{}

RapproximatedCurve::RapproximatedCurve(VectorPtr p0, VectorPtr p1, FeaturePtr orgCurve,
                                        VectorPtr tan0, VectorPtr tan1)
    : p0_(p0), p1_(p1), orgCurve_(orgCurve), tan0_(tan0), tan1_(tan1)
{}


void RapproximatedCurve::build()
{
    // Get the original edge and wrap it in an adaptor
    TopoDS_Edge e = orgCurve_->asSingleEdge();
    BRepAdaptor_Curve adapt(e);

    // Extract underlying Geom_Curve for point projection
    Standard_Real pf, pl;
    Handle_Geom_Curve geomCrv = BRep_Tool::Curve(e, pf, pl);

    // Project the new endpoints onto the original curve to find the clipped range
    gp_Pnt gp0 = to_Pnt(p0_->value());
    gp_Pnt gp1 = to_Pnt(p1_->value());

    GeomAPI_ProjectPointOnCurve proj0(gp0, geomCrv, pf, pl);
    GeomAPI_ProjectPointOnCurve proj1(gp1, geomCrv, pf, pl);

    if (proj0.NbPoints() == 0 || proj1.NbPoints() == 0)
        throw insight::Exception("RapproximatedCurve: could not project endpoints onto original curve");

    double u0 = proj0.LowerDistanceParameter();
    double u1 = proj1.LowerDistanceParameter();

    // Ensure u0 < u1; track swap so tangents are assigned to the correct end
    bool swapped = false;
    if (u0 > u1)
    {
        std::swap(u0, u1);
        std::swap(gp0, gp1);
        swapped = true;
    }

    // Resolve optional tangent constraints (account for endpoint swap)
    bool hasTan0 = swapped ? (bool)tan1_ : (bool)tan0_;
    bool hasTan1 = swapped ? (bool)tan0_ : (bool)tan1_;
    gp_Vec user_tan0 = hasTan0 ? to_Vec((swapped ? tan1_ : tan0_)->value()) : gp_Vec();
    gp_Vec user_tan1 = hasTan1 ? to_Vec((swapped ? tan0_ : tan1_)->value()) : gp_Vec();
    // Normalise so that the chord-length scaling (L_start / L_end) is the sole
    // magnitude factor in the Hermite; a non-unit user vector would otherwise
    // multiply L and produce an oversized tangent that causes the Hermite to loop.
    if (hasTan0) user_tan0.Normalize();
    if (hasTan1) user_tan1.Normalize();

    // Blend zone: first/last t_blend fraction of the clipped arc is the transition region.
    // Without a tangent constraint: smoothstep position blend (existing behaviour).
    // With a tangent constraint: Hermite cubic that simultaneously enforces position AND
    // tangent direction in the transition zone, so sample points are consistent with the
    // desired endpoint tangent before GeomAPI_Interpolate::Load() enforces it exactly.
    const int N = 12;
    const double t_blend = 0.25;

    auto smoothstep = [](double t) -> double {
        t = std::max(0.0, std::min(1.0, t));
        return t * t * (3.0 - 2.0 * t);
    };

    // Hermite cubic: p(t) = h00*p0 + h10*m0 + h01*p1 + h11*m1
    auto hermite = [](double t,
                      const gp_Pnt& p0h, const gp_Vec& m0h,
                      const gp_Pnt& p1h, const gp_Vec& m1h) -> gp_Pnt
    {
        double t2 = t * t, t3 = t * t * t;
        double h00 =  2*t3 - 3*t2 + 1;
        double h10 =    t3 - 2*t2 + t;
        double h01 = -2*t3 + 3*t2;
        double h11 =    t3 -   t2;
        return gp_Pnt(
            h00*p0h.X() + h10*m0h.X() + h01*p1h.X() + h11*m1h.X(),
            h00*p0h.Y() + h10*m0h.Y() + h01*p1h.Y() + h11*m1h.Y(),
            h00*p0h.Z() + h10*m0h.Z() + h01*p1h.Z() + h11*m1h.Z()
        );
    };

    // Precompute Hermite targets at the inner ends of each transition zone
    // Start zone ends at s=t_blend; end zone begins at s=1-t_blend
    gp_Pnt p_bs, p_be;
    gp_Vec tan_bs, tan_be;
    adapt.D1(u0 + (u1 - u0) * t_blend,         p_bs, tan_bs);
    adapt.D1(u0 + (u1 - u0) * (1.0 - t_blend), p_be, tan_be);
    tan_bs.Normalize();   // D1() gives raw parameter-space derivative; normalise before scaling
    tan_be.Normalize();
    double L_start = gp0.Distance(p_bs);   // chord length used to scale Hermite tangents
    double L_end   = gp1.Distance(p_be);

    // Debug: collect original-curve points and blended points as polylines
    // BRepBuilderAPI_MakePolygon poly_orig, poly_blend;

    Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt(1, N);
    for (int i = 0; i < N; i++)
    {
        double s = (double)i / (N - 1);
        double u = u0 + (u1 - u0) * s;
        gp_Pnt p_orig = adapt.Value(u);
        // poly_orig.Add(p_orig);
        gp_Pnt pos;

        if (s < t_blend)
        {
            double tl = s / t_blend;   // local 0→1 within start transition zone
            if (hasTan0)
            {
                // Hermite from (gp0, user_tan0) to (p_bs, tan_bs)
                // tangents scaled by chord length for well-conditioned interpolation
                pos = hermite(tl, gp0, L_start * user_tan0, p_bs, L_start * tan_bs);
            }
            else
            {
                double w = 1.0 - smoothstep(tl);
                pos = gp_Pnt(
                    w * gp0.X() + (1.0 - w) * p_orig.X(),
                    w * gp0.Y() + (1.0 - w) * p_orig.Y(),
                    w * gp0.Z() + (1.0 - w) * p_orig.Z()
                );
            }
        }
        else if (s > 1.0 - t_blend)
        {
            double tl = (s - (1.0 - t_blend)) / t_blend;   // local 0→1 within end zone
            if (hasTan1)
            {
                // Hermite from (p_be, tan_be) to (gp1, user_tan1)
                // m1 = L_end * user_tan1: arrival tangent at gp1 equals user_tan1
                pos = hermite(tl, p_be, L_end * tan_be, gp1, L_end * user_tan1);
            }
            else
            {
                double w = smoothstep(tl);
                pos = gp_Pnt(
                    (1.0 - w) * p_orig.X() + w * gp1.X(),
                    (1.0 - w) * p_orig.Y() + w * gp1.Y(),
                    (1.0 - w) * p_orig.Z() + w * gp1.Z()
                );
            }
        }
        else
        {
            pos = p_orig;   // middle section: exactly on original curve
        }

        pts_col->SetValue(i + 1, pos);
        // poly_blend.Add(pos);
    }

    // providedSubshapes_["originalCurvePts"] = Import::create(poly_orig.Wire());
    // providedSubshapes_["blendedPts"]        = Import::create(poly_blend.Wire());

    GeomAPI_Interpolate splbuilder(pts_col, false, 1e-6);
    // if (hasTan0 || hasTan1)
    // {
    //     // Scale user tangents to match the chord length of the adjacent sample segment.
    //     // GeomAPI_Interpolate uses chord-length parameterisation, so the tangent
    //     // magnitude at each endpoint must equal the adjacent chord length; a unit
    //     // tangent causes OCC to create a loop to satisfy the constraint.
    //     gp_Vec chord0(pts_col->Value(1), pts_col->Value(2));
    //     gp_Vec chord1(pts_col->Value(N - 1), pts_col->Value(N));
    //     gp_Vec eff_tan0 = hasTan0 ? user_tan0 * 0.5*chord0.Magnitude() : chord0;
    //     gp_Vec eff_tan1 = hasTan1 ? user_tan1 * 0.5*chord1.Magnitude() : chord1;
    //     splbuilder.Load(eff_tan0, eff_tan1);
    // }
    splbuilder.Perform();
    Handle_Geom_BSplineCurve crv = splbuilder.Curve();
    setShape(BRepBuilderAPI_MakeEdge(crv, crv->FirstParameter(), crv->LastParameter()));

    refpoints_["p0_proj"] = vec3(proj0.NearestPoint());
    refpoints_["p1_proj"] = vec3(proj1.NearestPoint());
}


void RapproximatedCurve::insertrule(parser::ISCADParser& ruleset)
{
    ruleset.modelstepFunctionRules.add
    (
        "RapproximatedCurve",
        std::make_shared<parser::ISCADParser::ModelstepRule>(

        ( '(' > ruleset.r_vectorExpression
          > ',' > ruleset.r_vectorExpression
          > ',' > ruleset.r_solidmodel_expression
          > ( ( ',' > qi::lit("der") > ruleset.r_vectorExpression > ruleset.r_vectorExpression )
            | ( qi::attr(VectorPtr()) >> qi::attr(VectorPtr()) ) )
          > ')' )
        [ qi::_val = phx::bind(
                         &RapproximatedCurve::create<VectorPtr, VectorPtr, FeaturePtr, VectorPtr, VectorPtr>,
                         qi::_1, qi::_2, qi::_3,
                         phx::at_c<0>(qi::_4), phx::at_c<1>(qi::_4)) ]

        )
    );
}


FeatureCmdInfoList RapproximatedCurve::ruleDocumentation()
{
    return {
        FeatureCmdInfo(
            "RapproximatedCurve",
            "( <vector:p0>, <vector:p1>, <feature:orgCurve> [, der <vector:tan0> <vector:tan1>] )",
            _("Creates a spline starting at p0 and ending at p1. "
              "Both endpoints are projected onto orgCurve; the curve follows "
              "orgCurve between those projections, with smooth transition "
              "wings blending from the new endpoints to their respective "
              "projection points on the original curve. "
              "Optionally, tangent vectors tan0 and tan1 can be supplied "
              "with the 'der' keyword to enforce departure/arrival directions "
              "at the new endpoints.")
        )
    };
}


VectorPtr RapproximatedCurve::start() const
{
    return p0_;
}

VectorPtr RapproximatedCurve::end() const
{
    return p1_;
}



}
}
