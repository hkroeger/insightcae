#ifndef INSIGHT_CAD_FEATUREFILTER_GRAMMAR_H
#define INSIGHT_CAD_FEATUREFILTER_GRAMMAR_H

/*
 * boost::spirit grammar of the feature filter expressions.
 * Only to be included by the translation units, which implement
 * the filter parsers (featurefilter*.cpp). Everyone else should use
 * the parse*FilterExpr functions from featurefilter.h.
 */

#include "featurefilter.h"

#include "boost/spirit/include/qi.hpp"
#include <boost/spirit/include/qi_no_case.hpp>

namespace insight
{
namespace cad
{

#ifndef SWIG
namespace qi = boost::spirit::qi;
using FeatureFilterIter = std::string::iterator;
using FeatureFilterSkipper = qi::space_type;




struct FeatureFilterExprParser
    : qi::grammar<FeatureFilterIter, FilterPtr(), qi::space_type>
{

public:
    qi::rule<iterator_type, std::string(), FeatureFilterSkipper> r_identifier;
    qi::rule<iterator_type, FeatureSetPtr(), FeatureFilterSkipper> r_featureset;
    qi::rule<iterator_type, FilterPtr(), FeatureFilterSkipper> r_filter_primary, r_filter_and, r_filter_or;
    qi::rule<iterator_type, FilterPtr(), FeatureFilterSkipper> r_filter;
    qi::rule<iterator_type, FilterPtr(), FeatureFilterSkipper> r_qty_comparison;
    qi::rule<iterator_type, scalarQuantityComputer::Ptr(), FeatureFilterSkipper> r_scalar_qty_expression, r_scalar_primary, r_scalar_term;
    qi::rule<iterator_type, matQuantityComputer::Ptr(), FeatureFilterSkipper> r_mat_qty_expression, r_mat_primary, r_mat_term;

    qi::rule<iterator_type, FilterPtr(), FeatureFilterSkipper> r_filter_functions;
    qi::rule<iterator_type, scalarQuantityComputer::Ptr(), FeatureFilterSkipper> r_scalar_qty_functions;
    qi::rule<iterator_type, matQuantityComputer::Ptr(), FeatureFilterSkipper> r_mat_qty_functions;

    FeatureSetParserArgList externalFeatureSets_;

    FeatureFilterExprParser(
        const FeatureSetParserArgList& extsets );

    static void featureFilterParseError(
        FeatureFilterIter /*first*/,
        FeatureFilterIter last,
        FeatureFilterIter error_pos,
        const boost::spirit::info& what);
};




template<class Parser>
FilterPtr parseFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
    Parser parser(refs);

    std::string contents_raw;
    readStreamIntoString(in, contents_raw);

    std::string::iterator first=contents_raw.begin();
    std::string::iterator last=contents_raw.end();

    FilterPtr result;
    bool r = qi::phrase_parse
        (
            first,
            last,
            parser,
            qi::space,
            result
            );


    if (first != last) // fail if we did not get a full match
    {
        int n=first-contents_raw.begin();
        throw insight::Exception(
            "parsing of filtering expression '%s' failed after %d chars!",
            contents_raw.c_str(), n );

        return FilterPtr();
    }

    return result;
}
#endif // SWIG

}
}

#endif // INSIGHT_CAD_FEATUREFILTER_GRAMMAR_H
