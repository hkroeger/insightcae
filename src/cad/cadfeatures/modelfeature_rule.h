#ifndef INSIGHT_CAD_MODELFEATURE_RULE_H
#define INSIGHT_CAD_MODELFEATURE_RULE_H

#include "parser.h"
#include "cadfeatures/modelfeature.h"

namespace insight {
namespace cad {

typedef qi::rule<
    std::string::iterator,
    std::shared_ptr<ModelFeature>(ModelVariableTable),
    insight::cad::parser::skip_grammar
    > ModelFeatureArgRule;

/**
 * @brief modelFeatureArgRule
 * returns the grammar rule for a sub model name plus variable overrides
 * (creates it in the ruleset on first call)
 */
const ModelFeatureArgRule& modelFeatureArgRule( parser::ISCADParser& ruleset );

}
}

#endif // INSIGHT_CAD_MODELFEATURE_RULE_H
