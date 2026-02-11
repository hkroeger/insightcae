#ifndef PARSER_TOOLS_H
#define PARSER_TOOLS_H

#include "parser.h"

namespace insight {
namespace cad {
namespace parser {


template<typename Map>
struct MapLookup
{
    const Map& map;

    MapLookup(const Map& m)
        : map(m)
    {}

    template<typename Attrib, typename Context>
    void operator()(Attrib& attr, Context& ctx, bool& success) const
    {
        std::string k(attr.begin(), attr.end());
        auto it = map.find(k);
        if (it == map.end())
        {
            success=false;
        }
        else
        {
            success=true;
            //qi::_val(ctx)=it->second;
            boost::fusion::at_c<0>(ctx.attributes)=
                it->second;
        }
    }
};


template<typename Map,typename It=std::string::iterator, typename Skipper=skip_grammar>
std::shared_ptr<qi::rule<It, typename Map::mapped_type(), Skipper> >
map_lookup_parser(const Map& map)
{
    return std::make_shared<qi::rule<It, typename Map::mapped_type(), Skipper> >(
        qi::as_string[ qi::lexeme[ qi::alpha >> *(qi::alnum | qi::char_('_')) ] ]
                        [ MapLookup(map) ]
        );
}

}
}
}

#endif // PARSER_TOOLS_H
