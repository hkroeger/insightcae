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
    return h.getHash();
}


RapproximatedCurve::RapproximatedCurve(const RapproximatedCurve& o, TreeCloneMap& tcm)
    : CL(p0_), CL(p1_), CL(orgCurve_)
{}

RapproximatedCurve::RapproximatedCurve(VectorPtr p0, VectorPtr p1, FeaturePtr orgCurve)
    : p0_(p0), p1_(p1), orgCurve_(orgCurve)
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

    // Ensure u0 < u1 so we always traverse the original curve forward
    if (u0 > u1)
    {
        std::swap(u0, u1);
        std::swap(gp0, gp1);
    }

    // Sample the clipped original curve [u0, u1] and blend each point's position
    // toward the new endpoint over the first/last t_blend fraction of the arc.
    //
    // smoothstep weight for start: w_start = 1 - smoothstep(s / t_blend)
    //   → 1 at s=0 (full gp0), 0 at s=t_blend (full original)
    // smoothstep weight for end:   w_end = 1 - smoothstep((1-s) / t_blend)
    //   → 0 at s=1-t_blend (full original), 1 at s=1 (full gp1)
    // blended pos = w_start*gp0 + w_end*gp1 + (1-w_start-w_end)*p_orig
    const int N = 50;
    const double t_blend = 0.25;

    auto smoothstep = [](double t) -> double {
        t = std::max(0.0, std::min(1.0, t));
        return t * t * (3.0 - 2.0 * t);
    };

    Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt(1, N);
    for (int i = 0; i < N; i++)
    {
        double s = (double)i / (N - 1);
        double u = u0 + (u1 - u0) * s;
        gp_Pnt p_orig = adapt.Value(u);

        double w_start = (s < t_blend) ? (1.0 - smoothstep(s / t_blend)) : 0.0;
        double w_end   = (s > 1.0 - t_blend) ? (1.0 - smoothstep((1.0 - s) / t_blend)) : 0.0;
        double w_orig  = 1.0 - w_start - w_end;

        pts_col->SetValue(i + 1, gp_Pnt(
            w_start * gp0.X() + w_end * gp1.X() + w_orig * p_orig.X(),
            w_start * gp0.Y() + w_end * gp1.Y() + w_orig * p_orig.Y(),
            w_start * gp0.Z() + w_end * gp1.Z() + w_orig * p_orig.Z()
        ));
    }

    GeomAPI_Interpolate splbuilder(pts_col, false, 1e-6);
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
          > ')' )
        [ qi::_val = phx::bind(
                         &RapproximatedCurve::create<VectorPtr, VectorPtr, FeaturePtr>,
                         qi::_1, qi::_2, qi::_3) ]

        )
    );
}


FeatureCmdInfoList RapproximatedCurve::ruleDocumentation()
{
    return {
        FeatureCmdInfo(
            "RapproximatedCurve",
            "( <vector:p0>, <vector:p1>, <feature:orgCurve> )",
            _("Creates a spline starting at p0 and ending at p1. "
              "Both endpoints are projected onto orgCurve; the curve follows "
              "orgCurve between those projections, with smooth transition "
              "wings blending from the new endpoints to their respective "
              "projection points on the original curve.")
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
