#ifndef CONSTRAINEDSKETCHGRAMMAR_H
#define CONSTRAINEDSKETCHGRAMMAR_H


#include <memory>
#include <functional>

#include "parser.h"



namespace insight {

class ParameterSet;

namespace cad {


class ConstrainedSketch;
class ConstrainedSketchEntity;
class ConstrainedSketchGeometry;
class ConstrainedSketchParametersDelegate;



/**
 * @brief The ParameterBinding struct
 * links a (top-level double) parameter of a sketch entity
 * to a scalar expression of the ISCAD script, which contains the sketch.
 */
struct ParameterBinding
{
    std::string expression; ///< expression text, as given in the script
    ScalarPtr value;

    ParameterBinding() = default;
    ParameterBinding(const std::string& expr, ScalarPtr v)
        : expression(expr), value(v)
    {}
};

typedef std::map<std::string, ParameterBinding> ParameterBindings;


struct ConstrainedSketchGrammar
    :  public ExtendedGrammar<qi::grammar<std::string::iterator, cad::parser::skip_grammar> >
{
public:
    typedef std::pair<int, std::shared_ptr<ConstrainedSketchEntity> > ParserRuleResult;
    typedef
        boost::spirit::qi::rule<
            std::string::iterator,
            ParserRuleResult(),
            insight::cad::parser::skip_grammar,
            boost::spirit::qi::locals<std::shared_ptr<ConstrainedSketchEntity> >
        > ParserRule;

    boost::spirit::qi::symbols<char, ParserRule> entityRules;

    std::shared_ptr<ConstrainedSketch> sketch;
    std::set<int> parsedEntityLabels;

    cad::parser::ISCADParser *iscadScriptRules;

    boost::spirit::qi::rule<
        std::string::iterator,
        VectorPtr(),
        insight::cad::parser::skip_grammar
        > r_vector;

    boost::spirit::qi::rule<
        std::string::iterator,
        VectorPtr(),
        insight::cad::parser::skip_grammar
        > r_point;

    boost::spirit::qi::rule<
        std::string::iterator,
        std::string(),
        insight::cad::parser::skip_grammar
        > r_label;

    boost::spirit::qi::rule<
        std::string::iterator,
        std::string(),
        insight::cad::parser::skip_grammar
        > r_parametersetstring;

    boost::spirit::qi::rule<
        std::string::iterator,
        std::string(),
        insight::cad::parser::skip_grammar
        > r_parameters;

    boost::spirit::qi::rule<
        std::string::iterator,
        std::pair<std::string, ParameterBinding>(),
        insight::cad::parser::skip_grammar,
        boost::spirit::qi::locals<ScalarPtr>
        > r_parameterBinding;

    /**
     * @brief r_parameterBindings
     * optional clause ", set ( <name> = <scalar expression>, ... )".
     * Only available, if the sketch is parsed as part of an ISCAD script.
     */
    boost::spirit::qi::rule<
        std::string::iterator,
        ParameterBindings(),
        insight::cad::parser::skip_grammar
        > r_parameterBindings;

    boost::spirit::qi::rule<
        std::string::iterator,
        insight::cad::parser::skip_grammar
        > r_layerProps;

    boost::spirit::qi::rule<
        std::string::iterator,
        std::shared_ptr<ConstrainedSketchEntity>(),
        insight::cad::parser::skip_grammar,
        boost::spirit::qi::locals<ParserRule>
        > r_entity;

    boost::spirit::qi::rule<
        std::string::iterator,
        insight::cad::parser::skip_grammar
        > r_sketch;

    ConstrainedSketchGrammar(
        std::shared_ptr<ConstrainedSketch> sk,
        const insight::cad::ConstrainedSketchParametersDelegate& pd,
        cad::parser::ISCADParser *iscadScriptRules = nullptr
        );

};

}
}

#endif // CONSTRAINEDSKETCHGRAMMAR_H
