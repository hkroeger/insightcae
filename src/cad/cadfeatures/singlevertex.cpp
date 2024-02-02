#include "singlevertex.h"
#include "base/translations.h"
#include "BRepBuilderAPI_MakeVertex.hxx"


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

namespace insight {
namespace cad {

defineType(SingleVertex);
addToStaticFunctionTable(Feature, SingleVertex, insertrule);
addToStaticFunctionTable(Feature, SingleVertex, ruleDocumentation);

size_t SingleVertex::calcHash() const
{
    ParameterListHash h;
    h+=this->type();
    h+=p_->value();
    return h.getHash();
}

SingleVertex::SingleVertex(VectorPtr p)
    : p_(p)
{}

void SingleVertex::build()
{
    setShape ( BRepBuilderAPI_MakeVertex(to_Pnt(p_->value())).Vertex() );
}

void SingleVertex::insertrule(parser::ISCADParser& ruleset)
{
    ruleset.modelstepFunctionRules.add
        (
            "SingleVertex",
            std::make_shared<parser::ISCADParser::ModelstepRule>(
            ( '(' > ruleset.r_vectorExpression > ')' )
                [ qi::_val = phx::bind(
                         &SingleVertex::create<VectorPtr>,
                         qi::_1) ]
                )
            );
}

FeatureCmdInfoList SingleVertex::ruleDocumentation()
{
    return { (
            FeatureCmdInfo
            (
                "SingleVertex",

                "( <vector:location> )",

                _("Creates a topological vertex at the specified location.")
            )
        ) };
}

} // namespace cad
} // namespace insight
