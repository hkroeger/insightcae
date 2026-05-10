#include "featurefilter.h"
#include <boost/spirit/include/phoenix.hpp>

#include "quantitycomputers/solidcog.h"
#include "quantitycomputers/solidvolume.h"

#include "featurefilters/same.h"

namespace insight
{
namespace cad
{

namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;

struct SolidFeatureFilterExprParser
: public FeatureFilterExprParser
{

  SolidFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser(extsets)
  {
      FeatureFilterExprParser::r_filter_functions =
      ( qi::lit("isSame") > '(' > FeatureFilterExprParser::r_featureset > ')' )
        [ qi::_val = phx::bind(&sameSolid::create<FeatureSet>, *qi::_1) ]
      ;

      FeatureFilterExprParser::r_mat_qty_functions =
        ( qi::lit("CoG") )
      [ qi::_val = phx::bind(&solidCoG::create<>) ]
      ;

      FeatureFilterExprParser::r_scalar_qty_functions =
        ( qi::lit("volume") )
      [ qi::_val = phx::bind(&solidVolume::create<>) ]
      ;

  }
};



FilterPtr parseSolidFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
    return parseFilterExpr<SolidFeatureFilterExprParser>(in, refs);
}



}
}