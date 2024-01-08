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
        > r_parameters;

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
        std::function<insight::ParameterSet(void)> mdpf,
        cad::parser::ISCADParser *iscadScriptRules = nullptr
        );

};

}
}

#endif // CONSTRAINEDSKETCHGRAMMAR_H
