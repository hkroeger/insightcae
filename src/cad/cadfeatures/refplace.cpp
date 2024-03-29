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
#include "base/tools.h"
#include "base/translations.h"

//#include <dlib/optimization.h>

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
//addToFactoryTable(Feature, RefPlace);
addToStaticFunctionTable(Feature, RefPlace, insertrule);
addToStaticFunctionTable(Feature, RefPlace, ruleDocumentation);


const double dist_scale=1e-3;




gp_Trsf trsf_from_vector(const arma::mat& pp)
{
    gp_Trsf t; // final transform
    
    double dx=pp(3), dy=pp(4), dz=pp(5);
    double magd = ::sqrt(dx*dx+dy*dy+dz*dz);
    double theta = magd;
    double w = ::cos(0.5*theta);
    double
        vx=dx*sin(0.5*theta)/magd,
        vy=dy*sin(0.5*theta)/magd,
        vz=dz*sin(0.5*theta)/magd;
    
    const double w2 = w*w;
    const double x2 = vx*vx;
    const double y2 = vy*vy;
    const double z2 = vz*vz;

    const double txy = 2.*vx*vy;
    const double twz = 2.*w*vz;
    const double txz = 2.*vx*vz;
    const double twy = 2.*w*vy;
    const double tyz = 2.*vy*vz;
    const double twx = 2.*w*vx;

    t.SetValues(
        w2 + x2 - y2 - z2,  txy - twz,          txz + twy,          pp(0),
        txy + twz,          w2 - x2 + y2 - z2,  tyz - twx,          pp(1),
        txz - twy,          tyz + twx,          w2 - x2 - y2 + z2,  pp(2)
    );

    return t;
}


double Condition::residual(const arma::mat& values) const
{
    return residual( trsf_from_vector(values) );
}



CoincidentPoint::CoincidentPoint(VectorPtr p_org, VectorPtr p_targ)
: p_org_(p_org), p_targ_(p_targ)
{}


double CoincidentPoint::residual(const gp_Trsf& tr) const
{
    return pow( to_Pnt(p_org_->value()).Transformed(tr).Distance( to_Pnt(p_targ_->value()) )*dist_scale, 2);
}




ParallelAxis::ParallelAxis(VectorPtr dir_org, VectorPtr dir_targ)
: dir_org_(dir_org), dir_targ_(dir_targ)
{}


double ParallelAxis::residual(const gp_Trsf& tr) const
{
    return pow(to_Vec(dir_org_->value()).Transformed(tr).Angle( to_Vec(dir_targ_->value()) ), 2);
}




ParallelPlanes::ParallelPlanes(DatumPtr pl_org, DatumPtr pl_targ, Orientation orient)
: pl_org_(pl_org), pl_targ_(pl_targ), orient_(orient)
{}


double ParallelPlanes::residual(const gp_Trsf& tr) const
{
    gp_Pln pl = gp_Pln(pl_org_->plane()).Transformed(tr);
    gp_Pln ptarg(pl_targ_->plane());
//     std::cerr<<"sqdist="<<pl.SquareDistance(ptarg.Location())<<std::endl;
//     std::cerr<<"angle="<<pl.Axis().Direction().Angle(ptarg.Axis().Direction())*180./M_PI<<std::endl;
    if (orient_==Inverted)
      return pow(1. + pl.Axis().Direction().Dot(ptarg.Axis().Direction()), 2);
    else if  (orient_==Same)
      return pow(1. - pl.Axis().Direction().Dot(ptarg.Axis().Direction()), 2);
    else
      return pow(1. - fabs( pl.Axis().Direction().Dot(ptarg.Axis().Direction()) ), 2);
}




AlignedPlanes::AlignedPlanes(DatumPtr pl_org, DatumPtr pl_targ, Orientation orient)
: ParallelPlanes(pl_org, pl_targ, orient)
{}


double AlignedPlanes::residual(const gp_Trsf& tr) const
{
  double res = ParallelPlanes::residual(tr);

  gp_Pln pl = gp_Pln(pl_org_->plane()).Transformed(tr);
  gp_Pln ptarg(pl_targ_->plane());

  res += pl.SquareDistance(ptarg.Location())*pow(dist_scale,2);

  return res;
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
    
    gp_XYZ r = (ao.Location().XYZ()-at.Location().XYZ());
    r -= r.Dot(at.Direction().XYZ())*at.Direction().XYZ();

    return pow(ao.Direction().Angle(fac*at.Direction()), 2) + r.SquareModulus()*pow(dist_scale,2);
}
    
    
    
    
PointInPlane::PointInPlane(VectorPtr p_org, DatumPtr pl_targ)
: p_org_(p_org), pl_targ_(pl_targ)
{}


double PointInPlane::residual(const gp_Trsf& tr) const
{
    gp_Pnt pt = to_Pnt(p_org_->value()).Transformed(tr);
    gp_Pln pltarg(pl_targ_->plane());
//     std::cerr<<"sqdist="<<pltarg.SquareDistance(pt)<<std::endl;
    
    return pltarg.SquareDistance(pt)*pow(dist_scale,2);
}




PointOnAxis::PointOnAxis(VectorPtr p_org, DatumPtr ax_targ)
: p_org_(p_org), ax_targ_(ax_targ)
{}


double PointOnAxis::residual(const gp_Trsf& tr) const
{
    gp_Pnt pt = to_Pnt(p_org_->value()).Transformed(tr);
    gp_Ax1 axtarg=ax_targ_->axis();
    
    gp_XYZ r = pt.XYZ() - axtarg.Location().XYZ();
    r -= r.Dot(axtarg.Direction().XYZ())*axtarg.Direction().XYZ();
    return r.SquareModulus()*pow(dist_scale,2);
}


size_t RefPlace::calcHash() const
{
  ParameterListHash p;
  p+=*m_;

#warning hashes of placement rules missing

  return p.getHash();
}





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






//typedef dlib::matrix<double,0,1> column_vector;


void RefPlace::build()
{
    ExecTimer t("RefPlace::build() ["+featureSymbolName()+"]");

    if (conditions_.size()<=0)
    {
        throw insight::Exception("at least one condition has to be provided!");
    }

    if (!trsf_)
    {

//        class Obj
//        {
//        public:
//            mutable int iter=0;
//            const ConditionList& conditions;

//            Obj(const ConditionList& co) : conditions(co) {}
	    
//            double operator()(const column_vector& curplacement) const
//            {
//                iter++;
                
//                arma::mat x=arma::zeros(7);
//                for (int i=0; i<7; i++) x(i)=curplacement(i);

//                double Q=0.0;
//                for (size_t i=0; i<conditions.size(); i++)
//                {
//                    Q += conditions[i]->residual(x);
//                }
//                return Q;
//            }


//        } obj(conditions_);

        class Obj : public ObjectiveND
        {
        public:
            mutable int iter=0;
            const ConditionList& conditions;

            Obj(const ConditionList& co) : conditions(co) {}

            double operator()(const arma::mat& x) const
            {
                iter++;

                for (int i=0; i<6; i++)
                  std::cout<<x(i)<<" ";
                std::cout<<" /";

                double Q=0.0;
                for (size_t i=0; i<conditions.size(); i++)
                {
                    Q += conditions[i]->residual(x);
                }

                std::cout<<Q<<endl;
                return Q;
            }

            int numP() const { return 6; }

        } obj(conditions_);


        int n=6;
	
    arma::mat x0=arma::ones(n); //=vector_from_trsf(gp_Trsf());
        
//	column_vector starting_point(n);
//	for (int i=0; i<n; i++) starting_point(i)=x0(i);

//	double r = dlib::find_min_bobyqa(
//	  obj,
//	  starting_point,
//	  10,    // number of interpolation points
//	  dlib::uniform_matrix<double>(n,1, -1e100),  // lower bound constraint
//	  dlib::uniform_matrix<double>(n,1, 1e100),   // upper bound constraint
//	  10,    // initial trust region radius
//	  1e-8,  // stopping trust region radius
//	  100000    // max number of objective function evaluations
//	);

    arma::mat steps;
    steps << 1000.0 << 1000.0 << 1000.0 << 0.1 << 0.1 << 0.1;
        arma::mat tp=nonlinearMinimizeND(obj, x0, 1e-12, steps);
        double r=obj(tp);

	std::cout
	    <<"solved placement after "<<obj.iter<<" function calls, "
	      "residual r="<<r<<")."
	    <<std::endl;

//	arma::mat tp=arma::zeros(n);
//	for (int i=0; i<n; i++) tp(i)=starting_point(i);
	
//        arma::mat tp = nonlinearMinimizeND(obj, x0, 1e-10);
        trsf_.reset( new gp_Trsf(trsf_from_vector(tp)) );
    }

    setShape(BRepBuilderAPI_Transform(m_->shape(), *trsf_).Shape());
    copyDatumsTransformed(*m_, *trsf_, "", boost::assign::list_of("origin")("ex")("ez") );
    
}




void RefPlace::insertrule(parser::ISCADParser& ruleset)
{

    typedef qi::rule<
        std::string::iterator,
        ConditionPtr(),
        insight::cad::parser::skip_grammar
        > ConditionRule;

    ConditionRule& r_condition = ruleset.addAdditionalRule(
        std::make_shared<ConditionRule>());

    r_condition =
        (ruleset.r_vectorExpression >> qi::lit("==") >> ruleset.r_vectorExpression  )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<CoincidentPoint>(qi::_1, qi::_2)) ]
        |
        (ruleset.r_vectorExpression >> qi::lit("parallel") >> ruleset.r_vectorExpression  )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<ParallelAxis>(qi::_1, qi::_2)) ]
        |
        (ruleset.r_datumExpression >> qi::lit("planeparallel") >> ruleset.r_datumExpression
          >> ( ( qi::lit("inverted") >> qi::attr(ParallelPlanes::Inverted) ) | ( qi::lit("oriented") >> qi::attr(ParallelPlanes::Same) ) | qi::attr(ParallelPlanes::Undefined) ) )
          [ qi::_val = phx::construct<ConditionPtr>(phx::new_<ParallelPlanes>(qi::_1, qi::_2, qi::_3)) ]
        |
        (ruleset.r_datumExpression >> qi::lit("aligned") >> ruleset.r_datumExpression  
          >> ( ( qi::lit("inverted") >> qi::attr(AlignedPlanes::Inverted) ) | ( qi::lit("oriented") >> qi::attr(AlignedPlanes::Same) ) | qi::attr(AlignedPlanes::Undefined) ) )
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

                    ( '(' >> ruleset.r_solidmodel_expression >>
                      ',' >> r_condition % ','  >>
                      ')' )
                    [ qi::_val = phx::bind(
                         &RefPlace::create<FeaturePtr, ConditionList>,
                         qi::_1, qi::_2) ]

                ))
    );
}




FeatureCmdInfoList RefPlace::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "RefPlace",
            "( <feature:base>, [ <placement condition>, ...] )",
            _("Places the feature base by solving a set of placement conditions numerically (experimental).\n"
            "Available placement conditions are:\n"
            " - <vector> == <vector> (same point coordinates or vector components)\n"
            " - <vector> parallel <vector> (same vector orientation))\n"
            " - <datum> aligned <datum> [inverted] (direction properties of two datums are parallel or antiparallel. The latter holds, if keyword \"inverted\" is present)\n"
            " - <datum> inclined <datum> <scalar>\n"
            " - <datum> coaxial <datum> [inverted]\n"
            " - <vector> inplane <datum>\n"
              " - <vector> onaxis <datum>\n")
        )
    };
}



gp_Trsf RefPlace::transformation() const
{
  checkForBuildDuringAccess();
  return *trsf_;
}




}
}
