#include "constrainedsketchgrammar.h"


#include "sketch.h"

namespace insight {
namespace cad {

template <typename T>
struct lookupEntity_f
{
    struct result
    { typedef std::shared_ptr<T> type; };

    typename result::type operator()(
        const std::map<int, insight::cad::ConstrainedSketchEntityPtr>& labeledEntities,
        int i ) const
    {
        auto iter = labeledEntities.find(i);
        insight::assertion(
            iter!=labeledEntities.end(),
            "could not lookup sketch entity %d", i);
        auto targ = std::dynamic_pointer_cast<T>(iter->second);
        insight::assertion(
            bool(targ),
            "sketch entity %d is of unexpected type! Expected a %s, got a %s",
            i, T::typeName.c_str(), iter->second->type().c_str());
        return targ;
    }
};

template <typename T>
using lookupEntity_ = boost::phoenix::function<lookupEntity_f<T> >;




ConstrainedSketchGrammar::ConstrainedSketchGrammar(std::shared_ptr<ConstrainedSketch> sk)
    : ConstrainedSketchGrammar::base_type(r_sketch),
      sketch(sk)
{
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;

    r_parameters = (
        (',' > qi::lit("parameters") >
         qi::as_string[ qi::lexeme[
                            qi::string(std::string("<?xml"))
                            >> +(!qi::lit("</root>") >> qi::char_)
                            >> qi::string("</root>") ] ]
                      [ qi::_val = qi::_1, std::cout<<qi::_val<<std::endl ]
         )
        | qi::attr(std::string())
        );

    r_point =
        qi::int_
            [ qi::_val = lookupEntity_<insight::cad::Vector>()(phx::ref(labeledEntities), qi::_1) ]

        | ( '[' > qi::double_ > ',' > qi::double_ > ',' > qi::double_ > ']' )
            [ qi::_val = phx::bind(&vec3const, qi::_1, qi::_2, qi::_3) ]
        ;

    for (const auto& apr : *ConstrainedSketchEntity::addParserRuleFunctions_)
    {
        apr.second(*this);
    }

    r_entity =
        boost::spirit::qi::omit[ entityRules [qi::_a = qi::_1 ] ]
        >> qi::lazy(qi::_a) [ phx::insert(phx::ref(sk->geometry()), qi::_1) ];

    r_sketch =
        r_entity % ',';
}


}
}
