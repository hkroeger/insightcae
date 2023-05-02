#ifndef CONSTRAINEDSKETCHGRAMMAR_H
#define CONSTRAINEDSKETCHGRAMMAR_H


#include <memory>

#include "parser.h"



namespace insight {
namespace cad {


class ConstrainedSketch;
class ConstrainedSketchEntity;
class ConstrainedSketchGeometry;

struct ConstrainedSketchGrammar
 :  boost::spirit::qi::grammar<std::string::iterator, parser::skip_grammar>
{
public:
    typedef
        boost::spirit::qi::rule<std::string::iterator, std::shared_ptr<ConstrainedSketchEntity>(), insight::cad::parser::skip_grammar>
            ParserRule;

    typedef std::map<int, std::shared_ptr<ConstrainedSketchEntity> > LabeledEntitiesMap;

    std::shared_ptr<ConstrainedSketch> sketch;
    LabeledEntitiesMap labeledEntities;
    boost::spirit::qi::symbols<char, ParserRule> entityRules;

    boost::spirit::qi::rule<std::string::iterator, VectorPtr(), insight::cad::parser::skip_grammar> r_point;
    boost::spirit::qi::rule<std::string::iterator, std::string(), insight::cad::parser::skip_grammar> r_parameters;
    boost::spirit::qi::rule<std::string::iterator, std::shared_ptr<ConstrainedSketchEntity>(), insight::cad::parser::skip_grammar, boost::spirit::qi::locals<ParserRule> > r_entity;
    boost::spirit::qi::rule<std::string::iterator, insight::cad::parser::skip_grammar> r_sketch;

    ConstrainedSketchGrammar(std::shared_ptr<ConstrainedSketch> sk);
};

}
}

#endif // CONSTRAINEDSKETCHGRAMMAR_H
