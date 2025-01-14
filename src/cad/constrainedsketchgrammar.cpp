#include "constrainedsketchgrammar.h"
#include "constrainedsketch.h"


namespace insight {
namespace cad {




ConstrainedSketchGrammar::ConstrainedSketchGrammar(
    std::shared_ptr<ConstrainedSketch> sk,
    const ConstrainedSketchParametersDelegate& pd,
    cad::parser::ISCADParser *isr
    )
    : ExtendedGrammar<qi::grammar<std::string::iterator, cad::parser::skip_grammar> >(r_sketch),
      sketch(sk),
      iscadScriptRules(isr)
{
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;

    r_parametersetstring =
         qi::as_string[ qi::lexeme[
             qi::string(std::string("<?xml"))
             >> +(!qi::lit("</root>") >> qi::char_)
             >> qi::string("</root>") ] ]
                      [ qi::_val = qi::_1 ]
        ;

    r_parameters = (
        ( ',' > qi::lit("parameters")
              > r_parametersetstring [ qi::_val = qi::_1 ] )
        | qi::attr(std::string())
        );

    r_label = qi::lexeme[ qi::alpha >> *(qi::alnum | qi::char_('_')) >> !(qi::alnum | '_') ];

    r_vector =
        ( '[' > qi::double_ > ',' > qi::double_ > ',' > qi::double_ > ']' )
            [ qi::_val = phx::bind(&vec3const, qi::_1, qi::_2, qi::_3) ]
        ;

    r_point =
        qi::int_
            [ qi::_val = phx::bind(&ConstrainedSketch::get<Vector>, sk, qi::_1) ]
        |
        r_vector [ qi::_val = qi::_1 ]
        ;

    r_layerProps =
        *( ( qi::lit("layer")
                     > r_label
                     > r_parametersetstring )
                        [ //std::cout<<"lbl="<<qi::_1<<std::endl,
                         phx::bind( &ConstrainedSketch::parseLayerProperties, sketch,
                                    qi::_1, qi::_2, pd ) ]
        )
        ;

    r_entity =
        boost::spirit::qi::omit[ entityRules [qi::_a = qi::_1 ] ]
        >> qi::lazy(qi::_a)
          [ /*phx::insert(phx::ref(sk->geometry()), qi::_1)*/
            phx::bind( &ConstrainedSketch::insertGeometry, sketch,
                              phx::bind(&ParserRuleResult::second, qi::_1),
                              phx::bind(&ParserRuleResult::first, qi::_1) ),
            phx::insert( phx::ref(parsedEntityLabels),
                                phx::bind(&ParserRuleResult::first, qi::_1))
          ];


    r_sketch =
        r_layerProps
        >>
        r_entity % ',';


    for (const auto& apr : *ConstrainedSketchEntity::addParserRuleFunctions_)
    {
        apr.second(*this, pd);
    }
}


}
}
