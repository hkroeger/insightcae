#ifndef PARSER_TOOLS_H
#define PARSER_TOOLS_H

#include "parser.h"

namespace insight {
namespace cad {
namespace parser {


/**
 * matches the keyword s only, if it is not immediately followed
 * by another identifier character. E.g. kw("in") does not match
 * the beginning of "inplane".
 */
inline auto kw(const char* s)
{
    return boost::proto::deep_copy(
        qi::lexeme[ qi::lit(s) >> !(qi::alnum | qi::char_('_')) ] );
}


/**
 * real number parser, which does not accept "inf" or "nan".
 * Otherwise identifiers like "infill" or "nanometer" would be
 * partially consumed as numbers.
 */
template<typename T>
struct iscad_real_policies
    : qi::real_policies<T>
{
    template <typename Iterator, typename Attribute>
    static bool parse_nan(Iterator&, Iterator const&, Attribute&)
    {
        return false;
    }

    template <typename Iterator, typename Attribute>
    static bool parse_inf(Iterator&, Iterator const&, Attribute&)
    {
        return false;
    }
};

const qi::real_parser<double, iscad_real_policies<double> > iscad_double
    = qi::real_parser<double, iscad_real_policies<double> >();




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
