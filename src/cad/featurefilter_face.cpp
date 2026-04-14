#include "featurefilter.h"
#include <boost/spirit/include/phoenix.hpp>

#include "quantitycomputers/cylradius.h"
#include "quantitycomputers/facearea.h"
#include "quantitycomputers/cylaxis.h"
#include "quantitycomputers/facenormalvector.h"
#include "quantitycomputers/facecog.h"

#include "featurefilters/facetopology.h"
#include "featurefilters/commonfacearea.h"
#include "featurefilters/ispartofface.h"
#include "featurefilters/ispartofsolid.h"
#include "featurefilters/coincident.h"
#include "featurefilters/identical.h"
#include "featurefilters/same.h"
#include "featurefilters/faceadjacenttoedges.h"
#include "featurefilters/faceadjacenttofaces.h"
#include "featurefilters/connected.h"
#include "quantitycomputers/constantquantity.h"


namespace insight
{
namespace cad
{

namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;


struct FaceFeatureFilterExprParser
: public FeatureFilterExprParser
{

  FaceFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser(extsets)
  {
    FeatureFilterExprParser::r_filter_functions =
    ( lit("isPlane") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_Plane) ]
    |
    ( lit("isCylinder") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_Cylinder) ]
    |
    ( lit("isCone") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_Cone) ]
    |
    ( lit("isSphere") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_Sphere) ]
    |
    ( lit("isTorus") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_Torus) ]
    |
    ( lit("isBezierSurface") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_BezierSurface) ]
    |
    ( lit("isBSplineSurface") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_BSplineSurface) ]
    |
    ( lit("isSurfaceOfRevolution") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_SurfaceOfRevolution) ]
    |
    ( lit("isSurfaceOfExtrusion") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_SurfaceOfExtrusion) ]
    |
    ( lit("isOffsetSurface") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_OffsetSurface) ]
    |
    ( lit("isOtherSurface") )
      [ qi::_val = phx::bind(&faceTopology::create<GeomAbs_SurfaceType>, GeomAbs_OtherSurface) ]
    |
    ( lit("hasCommonArea") > '(' > FeatureFilterExprParser::r_featureset  > ')')
      [ qi::_val = phx::bind(&hasCommonFaceArea::create<FeatureSetPtr>, qi::_1) ]
    |
    ( lit("isPartOfFace") > '(' > FeatureFilterExprParser::r_featureset  > ')')
      [ qi::_val = phx::bind(&isPartOfFaceFace::create<FeatureSet>, *qi::_1) ]
    |
    ( lit("isPartOfSolid") > '(' > FeatureFilterExprParser::r_featureset  > ')')
      [ qi::_val = phx::bind(&isPartOfSolidFace::create<FeatureSet>, *qi::_1) ]
    |
    ( lit("isCoincident") >> '('
            >> FeatureFilterExprParser::r_featureset
            >> ( ( ',' >> FeatureFilterExprParser::r_scalar_qty_expression ) | qi::attr(scalarQuantityComputerPtr(new constantQuantity<double>(1e-3))) )
            >> ')' )
      [ qi::_val = phx::bind(&coincidentFace::create<FeatureSet,scalarQuantityComputerPtr>, *qi::_1, qi::_2) ]
    |
    ( (lit("isIdentical")|lit("isCongruent")) > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&identicalFace::create<FeatureSet>, *qi::_1) ]
    |
    ( lit("isSame") > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&sameFace::create<FeatureSet>, *qi::_1) ]
    |
    ( lit("adjacentToEdges") > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&faceAdjacentToEdges::create<ConstFeatureSetPtr>, qi::_1) ]
    |
    ( lit("adjacentToFaces") > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&faceAdjacentToFaces::create<ConstFeatureSetPtr>, qi::_1) ]
    |
    ( lit("isConnectedTo") > '(' > FeatureFilterExprParser::r_featureset > ')' )
      [ qi::_val = phx::bind(&connectedFace::create<ConstFeatureSetPtr>, qi::_1) ]
    ;

    FeatureFilterExprParser::r_scalar_qty_functions =
    ( lit("cylRadius") )
      [ qi::_val = phx::bind(&cylRadius::create<>) ]
    |
    ( lit("area") )
      [ qi::_val = phx::bind(&faceArea::create<>) ]
    ;

    FeatureFilterExprParser::r_mat_qty_functions =
    ( lit("cylAxis") )
      [ qi::_val = phx::bind(&cylAxis::create<>) ]
    |
    ( lit("cylCenter") )
      [ qi::_val = phx::bind(&cylCenter::create<>) ]
    |
    ( lit("normal") )
      [ qi::_val = phx::bind(&faceNormalVector::create<>) ]
    |
    ( lit("CoG") )
      [ qi::_val = phx::bind(&faceCoG::create<>) ]
    ;

  }
};



FilterPtr parseFaceFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
    return parseFilterExpr<FaceFeatureFilterExprParser>(in, refs);
}



}
}