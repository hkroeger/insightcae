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

#include "refplace.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "gp_Quaternion.hxx"

#include <dlib/optimization.h>

#include "datum.h"

#include <chrono>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    

defineType(RefPlace);
addToFactoryTable(Feature, RefPlace);




gp_Trsf trsf_from_vector(const arma::mat& v)
{
    gp_Trsf t; // final transform
    
    /*
    //Euler angles
    gp_Trsf trsf1Probe, trsf2Probe, trsf3Probe;
    trsf1Probe.SetRotation(gp::OX(), v(0));
    trsf2Probe.SetRotation(gp::OY(), v(1));
    trsf3Probe.SetRotation(gp::OZ(), v(2));
    t = trsf1Probe * trsf2Probe * trsf3Probe;
    */
//     std::cerr<<"v="<<v<<std::endl;
    double vx=v(4);
    double mag=pow(vx,2)+pow(v(5),2)+pow(v(6),2);
    if (mag<1e-16) vx=1e-16;
//     gp_Quaternion q(vx, v(5), v(6), v(3));
//     t.SetRotation(q);
    t.SetRotation(gp_Ax1(gp_Pnt(0,0,0), gp_Dir(vx, v(5), v(6))), v(3));
    
    t.SetTranslationPart(gp_Vec(v(0), v(1), v(2)));
//     t.SetValues
//     (
//         v(0), v(1), v(2), v(3),
//         v(4), v(5), v(6), v(7),
//         v(8), v(9), v(10), v(11),
//         Precision::Confusion(), Precision::Confusion()
//     );    
    return t;
}

// arma::mat vector_from_trsf(const gp_Trsf& t)
// {
//     arma::mat v;
//     v
//      << t.Value(1,1) << t.Value(1,2) << t.Value(1,3) << t.Value(1,4)
//      << t.Value(2,1) << t.Value(2,2) << t.Value(2,3) << t.Value(2,4)
//      << t.Value(3,1) << t.Value(3,2) << t.Value(3,3) << t.Value(3,4) 
//      ;
//     return v;
// }


double Condition::residual(const arma::mat& values) const
{
    return residual( trsf_from_vector(values) );
}



CoincidentPoint::CoincidentPoint(VectorPtr p_org, VectorPtr p_targ)
: p_org_(p_org), p_targ_(p_targ)
{}


double CoincidentPoint::residual(const gp_Trsf& tr) const
{
    return pow( to_Pnt(p_org_->value()).Transformed(tr).Distance( to_Pnt(p_targ_->value()) ), 2);
}




ParallelAxis::ParallelAxis(VectorPtr dir_org, VectorPtr dir_targ)
: dir_org_(dir_org), dir_targ_(dir_targ)
{}


double ParallelAxis::residual(const gp_Trsf& tr) const
{
    return pow(to_Vec(dir_org_->value()).Transformed(tr).Angle( to_Vec(dir_targ_->value()) ), 2);
}




AlignedPlanes::AlignedPlanes(DatumPtr pl_org, DatumPtr pl_targ, bool inv)
: pl_org_(pl_org), pl_targ_(pl_targ), inv_(inv)
{}


double AlignedPlanes::residual(const gp_Trsf& tr) const
{
    gp_Pln pl = gp_Pln(pl_org_->plane()).Transformed(tr);
    gp_Pln ptarg(pl_targ_->plane());
//     std::cerr<<"sqdist="<<pl.SquareDistance(ptarg.Location())<<std::endl;
//     std::cerr<<"angle="<<pl.Axis().Direction().Angle(ptarg.Axis().Direction())*180./M_PI<<std::endl;
    if (inv_)
      return pow(M_PI-pl.Axis().Direction().Angle(ptarg.Axis().Direction()), 2) + pl.SquareDistance(ptarg.Location());
    else
      return pow(pl.Axis().Direction().Angle(ptarg.Axis().Direction()), 2) + pl.SquareDistance(ptarg.Location());
}




InclinedPlanes::InclinedPlanes(DatumPtr pl_org, DatumPtr pl_targ, ScalarPtr angle)
: pl_org_(pl_org), pl_targ_(pl_targ), angle_(angle)
{}


double InclinedPlanes::residual(const gp_Trsf& tr) const
{
    gp_Pln pl = gp_Pln(pl_org_->plane()).Transformed(tr);
    gp_Pln ptarg(pl_targ_->plane());
//     std::cerr<<"sqdist="<<pl.SquareDistance(ptarg.Location())<<std::endl;
//     std::cerr<<"angle="<<pl.Axis().Direction().Angle(ptarg.Axis().Direction())*180./M_PI<<std::endl;
    return ::pow( pl.Axis().Direction().Angle(ptarg.Axis().Direction()) - angle_->value(), 2);
}


Coaxial::Coaxial(DatumPtr ax_org, DatumPtr ax_targ, bool inv)
: ax_org_(ax_org), ax_targ_(ax_targ), inv_(inv)
{
}


double Coaxial::residual(const gp_Trsf& tr) const
{
    gp_Ax1 ao = ax_org_->axis().Transformed(tr);
    gp_Ax1 at = ax_targ_->axis();
    
    double fac=1.0;
    if (inv_) fac=-1.;
    
    gp_XYZ r = ao.Location().XYZ()-at.Location().XYZ();
    r -= r.Dot(at.Direction().XYZ())*at.Direction().XYZ();

    return pow(ao.Direction().Angle(fac*at.Direction()), 2) + r.SquareModulus();
}
    
    
    
    
PointInPlane::PointInPlane(VectorPtr p_org, DatumPtr pl_targ)
: p_org_(p_org), pl_targ_(pl_targ)
{}


double PointInPlane::residual(const gp_Trsf& tr) const
{
    gp_Pnt pt = to_Pnt(p_org_->value()).Transformed(tr);
    gp_Pln pltarg(pl_targ_->plane());
//     std::cerr<<"sqdist="<<pltarg.SquareDistance(pt)<<std::endl;
    
    return pltarg.SquareDistance(pt);
}




PointOnAxis::PointOnAxis(VectorPtr p_org, DatumPtr ax_targ)
: p_org_(p_org), ax_targ_(ax_targ)
{}


double PointOnAxis::residual(const gp_Trsf& tr) const
{
    gp_Pnt pt = to_Pnt(p_org_->value()).Transformed(tr);
    gp_Ax1 axtarg=ax_targ_->axis();
    
    gp_XYZ r = pt.XYZ()-axtarg.Location().XYZ();
    r -= r.Dot(axtarg.Direction().XYZ())*axtarg.Direction().XYZ();
    return r.SquareModulus();
}



RefPlace::RefPlace(): DerivedFeature()
{}




RefPlace::RefPlace(FeaturePtr m, const gp_Ax2& cs)
: DerivedFeature(m), m_(m) 
{
  trsf_.reset(new gp_Trsf);
  trsf_->SetTransformation(gp_Ax3(cs));
  trsf_->Invert();
}




RefPlace::RefPlace(FeaturePtr m, ConditionList conditions)
: DerivedFeature(m), m_(m), conditions_(conditions)
{}




FeaturePtr RefPlace::create_fix ( FeaturePtr m, const gp_Ax2& cs )
{
    return FeaturePtr(new RefPlace(m, cs));
}




FeaturePtr RefPlace::create ( FeaturePtr m, ConditionList conditions )
{
    return FeaturePtr(new RefPlace(m, conditions));
}



typedef dlib::matrix<double,0,1> column_vector;


void RefPlace::build()
{
    auto t_start = std::chrono::high_resolution_clock::now();

    if (conditions_.size()<=0)
    {
        throw insight::Exception("at least one condition has to be provided!");
    }

    if (!trsf_)
    {
//         class Obj : public ObjectiveND
//         {
//         public:
//             mutable int iter=0;
//             const ConditionList& conditions;
// 
//             Obj(const ConditionList& co) : conditions(co) {} ;
//             virtual double operator()(const arma::mat& x) const
//             {
//                 double Q=0.0;
//                 for (size_t i=0; i<conditions.size(); i++)
//                 {
//                     Q += conditions[i]->residual(x);
//                 }
// //                 std::cerr<<"i="<<(iter++)<<" x:"<<x<<" -> Q="<<Q<<std::endl;
//                 return Q;
//             }
// 
//             virtual int numP() const {
//                 return 7;
//             };
// 
//         } obj(conditions_);

        class Obj
        {
        public:
            mutable int iter=0;
            const ConditionList& conditions;

            Obj(const ConditionList& co) : conditions(co) {} ;
	    
            double operator()(const column_vector& curplacement) const
            {
		iter++;
	        arma::mat x=arma::zeros(7);
		for (int i=0; i<7; i++) x(i)=curplacement(i);
		
                double Q=0.0;
                for (size_t i=0; i<conditions.size(); i++)
                {
                    Q += conditions[i]->residual(x);
                }
//                 std::cerr<<"i="<<(iter++)<<" x:"<<x<<" -> Q="<<Q<<std::endl;
                return Q;
            }


        } obj(conditions_);

	int n=7;
	
        arma::mat x0=arma::zeros(n); //=vector_from_trsf(gp_Trsf());
        
	column_vector starting_point(n);
	for (int i=0; i<n; i++) starting_point(i)=x0(i);
	auto ts_start = std::chrono::high_resolution_clock::now();
	double r = dlib::find_min_bobyqa(
	  obj, 
	  starting_point, 
	  10,    // number of interpolation points
	  dlib::uniform_matrix<double>(n,1, -1e100),  // lower bound constraint
	  dlib::uniform_matrix<double>(n,1, 1e100),   // upper bound constraint
	  10,    // initial trust region radius
	  1e-6,  // stopping trust region radius
	  100000    // max number of objective function evaluations
	);
	auto ts_end = std::chrono::high_resolution_clock::now();
	std::cout<<"solved placement after "<<obj.iter<<" function calls "
	   "("<<std::chrono::duration_cast<std::chrono::milliseconds>(ts_end - ts_start).count()<<"ms)"
	   "."<<std::endl;
	arma::mat tp=arma::zeros(n);
	for (int i=0; i<n; i++) tp(i)=starting_point(i);
	
//        arma::mat tp = nonlinearMinimizeND(obj, x0, 1e-10);
        trsf_.reset( new gp_Trsf(trsf_from_vector(tp)) );
    }

    setShape(BRepBuilderAPI_Transform(m_->shape(), *trsf_).Shape());
    copyDatumsTransformed(*m_, *trsf_, "", boost::assign::list_of("origin")("ex")("ez") );
    
    auto t_end = std::chrono::high_resolution_clock::now();
    
    std::cout << "RefPlace rebuild done in " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count()
              << " ms" << std::endl;
}




void RefPlace::insertrule(parser::ISCADParser& ruleset) const
{

    typedef qi::rule<std::string::iterator, ConditionPtr(), insight::cad::parser::skip_grammar > ConditionRule;
    typedef insight::cad::parser::AddRuleContainer<ConditionRule> ConditionRuleContainer;

    ruleset.additionalrules_.push_back
    (
        new ConditionRuleContainer(new ConditionRule)
    );
    ConditionRule& r_condition = *(dynamic_cast<ConditionRuleContainer&>(ruleset.additionalrules_.back()));

    r_condition =
        (ruleset.r_vectorExpression >> qi::lit("==") >> ruleset.r_vectorExpression  )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<CoincidentPoint>(qi::_1, qi::_2)) ]
        |
        (ruleset.r_vectorExpression >> qi::lit("parallel") >> ruleset.r_vectorExpression  )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<ParallelAxis>(qi::_1, qi::_2)) ]
        |
        (ruleset.r_datumExpression >> qi::lit("aligned") >> ruleset.r_datumExpression  
          >> ( ( qi::lit("inverted") >> qi::attr(true) ) | qi::attr(false) ) )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<AlignedPlanes>(qi::_1, qi::_2, qi::_3)) ]
        |
        (ruleset.r_datumExpression >> qi::lit("inclined") >> ruleset.r_datumExpression >> ruleset.r_scalarExpression )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<InclinedPlanes>(qi::_1, qi::_2, qi::_3)) ]
        |
        (ruleset.r_datumExpression >> qi::lit("coaxial") >> ruleset.r_datumExpression 
	  >> ( ( qi::lit("inverted") >> qi::attr(true) ) | qi::attr(false) ) )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<Coaxial>(qi::_1, qi::_2, qi::_3)) ]
        |
        (ruleset.r_vectorExpression >> qi::lit("inplane") >> ruleset.r_datumExpression  )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<PointInPlane>(qi::_1, qi::_2)) ]
        |
        (ruleset.r_vectorExpression >> qi::lit("onaxis") >> ruleset.r_datumExpression  )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<PointOnAxis>(qi::_1, qi::_2)) ]
        ;
    r_condition.name("placement condition");

    ruleset.modelstepFunctionRules.add
    (
        "RefPlace",
        typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

                    ( '(' > ruleset.r_solidmodel_expression >
                      ',' > r_condition % ','  >
                      ')' )
                    [ qi::_val = phx::bind(&RefPlace::create, qi::_1, qi::_2) ]

                ))
    );
}




FeatureCmdInfoList RefPlace::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "RefPlace",
            "( <feature:base>, [ <placement condition>, ...] )",
            "Places the feature base by solving a set of placement conditions numerically (experimental)."
        )
    );
}



gp_Trsf RefPlace::transformation() const
{
  checkForBuildDuringAccess();
  return *trsf_;
}




}
}
