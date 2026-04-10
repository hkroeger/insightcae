
#include "featurefilter.h"
#include "cadparameters/constantvector.h"

#include <boost/spirit/include/phoenix.hpp>

#include "quantitycomputers/vertexlocation.h"


namespace insight
{
namespace cad
{

namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;


struct VertexFeatureFilterExprParser
: public FeatureFilterExprParser
{

  VertexFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser(extsets)
  {

      FeatureFilterExprParser::r_mat_qty_functions =
        ( qi::lit("loc") )
      [ qi::_val = phx::bind(&vertexLocation::create<>) ]
      ;

  }
};


FilterPtr parseVertexFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
    return parseFilterExpr<VertexFeatureFilterExprParser>(in, refs);
}



}
}
