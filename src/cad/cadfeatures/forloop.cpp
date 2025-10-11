#include "forloop.h"
#include "base/exception.h"
#include "boost/phoenix/stl/algorithm/transformation.hpp"
#include <boost/phoenix/stl/algorithm/iteration.hpp>
#include "cadfeature.h"
#include "cadmodel.h"
#include "datum.h"
#include "base/tools.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include <memory>

#include "base/translations.h"


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(ForLoop);
//addToFactoryTable(Feature, ModelFeature);
addToStaticFunctionTable(Feature, ForLoop, insertrule);
addToStaticFunctionTable(Feature, ForLoop, ruleDocumentation);



void
ForLoop::createInstances()
{

    int j=0;
    for (
        double i = i0_->value();
        i < imax_->value();
        i += increment_->value()
        )
    {
        ModelFeaturePtr inst =
            loopBodyTemplate_->deepClone<ModelFeature>();


        {
            // rewire loop var
            auto v=boost::get<ScalarPtr>(inst->findInputVariable(loopVarName_));
            insight::assertion(
                v, "requested variable %s not found in model", loopVarName_.c_str());
            inst->replaceDependency(
                DependencyReplacement(v->get(), cad::scalarconst(i)));
        }

        if (lastInstance_)
        {
            //if (auto v=boost::get<FeaturePtr>(inst->findInputVariable("PREV")))
            if (auto v=inst->findModelstep("PREV"))
            {
                inst->replaceDependency(
                    DependencyReplacement(v.get(), lastInstance_));
            }
        }

        components_[str(format("instance%d")%j)]=inst;

        lastInstance_=inst;
        j++;
    }


    invalidate();
}

ForLoop::ForLoop(const ForLoop &o, TreeCloneMap &tcm)
    : Compound(o, tcm),
    loopVarName_(o.loopVarName_),
    CL(i0_), CL(imax_), CL(increment_)
{}

ForLoop::ForLoop (
    const std::string& varname,
    ModelFeaturePtr model,
    ScalarPtr i0, ScalarPtr imax,
    ScalarPtr increment )
    : Compound(),
    loopVarName_(varname),
    loopBodyTemplate_(model),
    i0_(i0), imax_(imax), increment_(increment)
{}

size_t ForLoop::calcHash() const
{
    ParameterListHash h;
    h+=loopVarName_;
    h+=*i0_;
    h+=*imax_;
    h+=*increment_;
    return h.getHash()+Compound::calcHash();
}

void ForLoop::build()
{
    Compound::build();
    providedSubshapes_["instanceFinal"]=lastInstance_;
}


ModelVariableTable loopvar(const std::string&name, ScalarPtr i0)
{
    return ModelVariableTable
        {
            ModelVariableAndName{name, cad::scalarconst(i0->value())}
        };
}

void ForLoop::insertrule ( parser::ISCADParser& ruleset )
{
    auto &for_loop_rule =
        ruleset.addAdditionalRule(
        new         qi::rule<std::string::iterator,
        FeaturePtr(),
        parser::skip_grammar,
        qi::locals<std::string,ScalarPtr> >(
        '('
        > ruleset.r_identifier [qi::_a = qi::_1 ] > ','
        > ruleset.r_scalarExpression [ qi::_b = qi::_1 ] > ','
        > ( ruleset.r_scalarExpression > ','
           > ( (qi::lit("inc") > ruleset.r_scalarExpression > ',')|qi::attr(cad::scalarconst(1.)))
           > (
               ruleset.r_submodel ( phx::bind(&loopvar, qi::_a, qi::_b) )
               |
               ModelFeature::rule(ruleset) ( phx::bind(&loopvar, qi::_a, qi::_b) )
               )
           > ')' )
            [ qi::_val = phx::bind (
                 &ForLoop::create<const std::string&, ModelFeaturePtr, ScalarPtr, ScalarPtr, ScalarPtr>,
                 qi::_a, qi::_3, qi::_b, qi::_1, qi::_2 ) ]
            ));

    ruleset.modelstepFunctionRules.add
        ("for",
         std::make_shared<parser::ISCADParser::ModelstepRule>(for_loop_rule) );
}


FeatureCmdInfoList ForLoop::ruleDocumentation()
{
    return {};
}


} // namespace cad
} // namespace insight
