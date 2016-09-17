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

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    

defineType(RefPlace);
addToFactoryTable(Feature, RefPlace, NoParameters);

gp_Trsf trsf_from_vector(const arma::mat& v)
{
    gp_Trsf t;
    t.SetValues
    (
        v(0), v(1), v(2), v(3),
        v(4), v(5), v(6), v(7),
        v(8), v(9), v(10), v(11),
        Precision::Confusion(), Precision::Confusion()
    );    
    return t;
}

arma::mat vector_from_trsf(const gp_Trsf& t)
{
    arma::mat v;
    v
     << t.Value(1,1) << t.Value(1,2) << t.Value(1,3) << t.Value(1,4)
     << t.Value(2,1) << t.Value(2,2) << t.Value(2,3) << t.Value(2,4)
     << t.Value(3,1) << t.Value(3,2) << t.Value(3,3) << t.Value(3,4) 
     ;
    return v;
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
    return to_Pnt(p_org_->value()).Transformed(tr).Distance( to_Pnt(p_targ_->value()) );
}




RefPlace::RefPlace(const NoParameters& nop): DerivedFeature(nop)
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



void RefPlace::build()
{

    if (conditions_.size()<=0)
    {
        throw insight::Exception("at least one condition has to be provided!");
    }

    if (!trsf_)
    {
        class Obj : public ObjectiveND
        {
        public:
            const ConditionList& conditions;

            Obj(const ConditionList& co) : conditions(co) {} ;
            virtual double operator()(const arma::mat& x) const
            {
                double Q=0.0;
                for (size_t i=0; i<conditions.size(); i++)
                {
                    Q += pow(conditions[i]->residual(x), 2);
                }
                std::cerr<<"Q="<<Q<<std::endl;
                return Q;
            }

            virtual int numP() const {
                return 3*4;
            };

        } obj(conditions_);

        arma::mat tp = nonlinearMinimizeND(obj, vector_from_trsf(gp_Trsf()));
        trsf_.reset( new gp_Trsf(trsf_from_vector(tp)) );
    }

    setShape(BRepBuilderAPI_Transform(m_->shape(), *trsf_).Shape());

    copyDatumsTransformed(*m_, *trsf_);
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
        ;
    r_condition.name("placement condition");

    ruleset.modelstepFunctionRules.add
    (
        "RefPlace",
        typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

                    ( '(' > ruleset.r_solidmodel_expression >
                      ',' > r_condition % ','  >
                      ')' )
                    [ qi::_val = phx::construct<FeaturePtr>(phx::new_<RefPlace>(qi::_1, qi::_2)) ]

                ))
    );
}




gp_Trsf RefPlace::transformation() const
{
  checkForBuildDuringAccess();
  return *trsf_;
}




}
}
