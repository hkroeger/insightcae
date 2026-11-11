#ifndef INSIGHT_CAD_SPIRIT_FWD_H
#define INSIGHT_CAD_SPIRIT_FWD_H

/*
 * Lightweight forward declarations of boost::spirit types,
 * which are referenced in widely used CAD headers.
 *
 * Including <boost/spirit/include/qi.hpp> costs ~0.8 GB of
 * compiler memory per translation unit. Headers which only need
 * to mention these types should include this file instead.
 *
 * Note: the template parameters are declared without default
 * arguments here; the defaults are provided by the real
 * definition in boost/spirit/home/qi/string/symbols.hpp.
 * Thus, always spell out all four parameters when referring to
 * qi::symbols in a context where only this header is included.
 */

namespace boost {
namespace spirit {
namespace qi {

template <typename Char, typename T, typename Lookup, typename Filter>
struct symbols;

template <typename Char, typename T>
struct tst;

struct tst_pass_through;

/**
 * qi::symbols with its default lookup and filter types spelled out,
 * usable when only this header is included
 */
template <typename Char, typename T>
using default_symbols = symbols<Char, T, tst<Char, T>, tst_pass_through>;

} // namespace qi
} // namespace spirit
} // namespace boost

#endif // INSIGHT_CAD_SPIRIT_FWD_H
