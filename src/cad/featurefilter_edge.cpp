
#include "featurefilter.h"
#include <boost/spirit/include/phoenix.hpp>

#include "featurefilters/edgetopology.h"
#include "featurefilters/boundaryofface.h"
#include "featurefilters/boundaryedge.h"
#include "featurefilters/ispartofface.h"
#include "featurefilters/ispartofsolid.h"
#include "featurefilters/coincident.h"
#include "featurefilters/identical.h"
#include "featurefilters/same.h"
#include "featurefilters/coincidentprojectededge.h"
#include "featurefilters/edgeconnectingvertices.h"

#include "quantitycomputers/edgelen.h"
#include "quantitycomputers/edgeradiallen.h"
#include "quantitycomputers/edgecog.h"
#include "quantitycomputers/edgestart.h"
#include "quantitycomputers/edgeend.h"



namespace insight
{
namespace cad
{

namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;


struct EdgeFeatureFilterExprParser
: public FeatureFilterExprParser
{

  EdgeFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser(extsets)
  {
    FeatureFilterExprParser::r_filter_functions =
    ( lit("isLine") )
      [ qi::_val = phx::bind(&edgeTopology::create<GeomAbs_CurveType>, GeomAbs_Line) ]
    |
    ( lit("isCircle") )
      [ qi::_val = phx::bind(&edgeTopology::create<GeomAbs_CurveType>, GeomAbs_Circle) ]
    |
    ( lit("isEllipse") )
      [ qi::_val = phx::bind(&edgeTopology::create<GeomAbs_CurveType>, GeomAbs_Ellipse) ]
    |
    ( lit("isHyperbola") )
      [ qi::_val = phx::bind(&edgeTopology::create<GeomAbs_CurveType>, GeomAbs_Hyperbola) ]
    |
    ( lit("isParabola") )
      [ qi::_val = phx::bind(&edgeTopology::create<GeomAbs_CurveType>, GeomAbs_Parabola) ]
    |
    ( lit("isBezierCurve") )
      [ qi::_val = phx::bind(&edgeTopology::create<GeomAbs_CurveType>, GeomAbs_BezierCurve) ]
    |
    ( lit("isBSplineCurve") )
      [ qi::_val = phx::bind(&edgeTopology::create<GeomAbs_CurveType>, GeomAbs_BSplineCurve) ]
    |
    ( lit("isOtherCurve") )
      [ qi::_val = phx::bind(&edgeTopology::create<GeomAbs_CurveType>, GeomAbs_OtherCurve) ]
    |
    ( lit("isFaceBoundary") )
      [ qi::_val = phx::bind(&boundaryEdge::create<>) ]
    |
    ( lit("boundaryOfFace") > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&boundaryOfFace::create<FeatureSetPtr>, qi::_1) ]
    |
    ( lit("isPartOfSolid") > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&isPartOfSolidEdge::create<FeatureSet>, *qi::_1) ]
    |
    ( lit("isPartOfFace") > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&isPartOfFaceEdge::create<FeatureSet>, *qi::_1) ]
    |
    ( lit("isCoincident") >> '(' >> FeatureFilterExprParser::r_featureset
                          >> ( ( ',' >> FeatureFilterExprParser::r_scalar_qty_expression ) | qi::attr(scalarQuantityComputerPtr(new constantQuantity<double>(1e-3))) )
                          >> ')' )
      [ qi::_val = phx::bind(&coincidentEdge::create<FeatureSet,scalarQuantityComputerPtr>, *qi::_1, qi::_2) ]
    |
    ( (lit("isIdentical")|lit("isCongruent")) > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&identicalEdge::create<FeatureSet>, *qi::_1) ]
    |
    ( lit("isSame") > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&sameEdge::create<FeatureSet>, *qi::_1) ]
    |
    ( lit("projectionIsCoincident") > '('
      > FeatureFilterExprParser::r_featureset > ','
      > FeatureFilterExprParser::r_mat_qty_expression > ',' // p0
      > FeatureFilterExprParser::r_mat_qty_expression > ',' // n
      > FeatureFilterExprParser::r_mat_qty_expression > ','    // up
      > FeatureFilterExprParser::r_scalar_qty_expression > ')' // tol
    )
      [ qi::_val = phx::bind(
            &coincidentProjectedEdge::create<
                FeatureSet,
                const matQuantityComputerPtr&,
                const matQuantityComputerPtr&,
                const matQuantityComputerPtr&,
                const scalarQuantityComputerPtr&>,
            *qi::_1, qi::_2, qi::_3, qi::_4, qi::_5) ]
    |
     ( lit("connects") > '('
        > FeatureFilterExprParser::r_featureset > ','
        > FeatureFilterExprParser::r_featureset
        > ')' )
      [ qi::_val = phx::bind(
            &EdgeConnectingVertices::create<FeatureSetPtr,FeatureSetPtr>,
                    qi::_1, qi::_2 ) ]
      ;

      FeatureFilterExprParser::r_scalar_qty_functions =
    ( lit("len") )
      [ qi::_val = phx::bind(&edgeLen::create<>) ]
    |
    ( lit("circRadius") )
        [ qi::_val = phx::bind(&circRadius::create<>) ]
    |
    ( lit("radialLen") >
        '(' > FeatureFilterExprParser::r_mat_qty_expression > //ax
        ',' > FeatureFilterExprParser::r_mat_qty_expression >  //p0
        ')' )
      [ qi::_val = phx::bind(&edgeRadialLen::create<matQuantityComputerPtr,matQuantityComputerPtr>, qi::_1, qi::_2) ]
      ;

      FeatureFilterExprParser::r_mat_qty_functions =
//         ( lit("avgTangent") ) [ _val = phx::bind(&edgeAvgTangent::create<>) ]
//         |
        ( lit("CoG") )
      [ qi::_val = phx::bind(&edgeCoG::create<>) ]
        |
        ( lit("start") )
      [ qi::_val = phx::bind(&edgeStart::create<>) ]
        |
        ( lit("end") )
      [ qi::_val = phx::bind(&edgeEnd::create<>) ]
      ;

  }
};


FilterPtr parseEdgeFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
    return parseFilterExpr<EdgeFeatureFilterExprParser>(in, refs);
}


}
}