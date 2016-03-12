/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */



#include <memory>
#include "base/linearalgebra.h"
#include "featurefilter.h"
#include <base/exception.h>
#include "boost/foreach.hpp"
#include <boost/iterator/counting_iterator.hpp>
#include "boost/lexical_cast.hpp"

#include "cadfeature.h"

#define BOOST_SPIRIT_USE_PHOENIX_V3
// #define BOOST_SPIRIT_DEBUG

#include "base/boost_include.h"
#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

#include "datum.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{



namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;
using namespace insight::cad;


FeatureSetPtr lookupFeatureSet(const FeatureSetParserArgList& fl, size_t id)
{
  if (id>=fl.size())
    throw insight::Exception
    (
      "Feature set #"+lexical_cast<std::string>(id)
     +" is not present in list of size "+lexical_cast<std::string>(fl.size())
    );
  
  
  if (const FeatureSetPtr* fsp = boost::get<FeatureSetPtr>(&fl.at(id)))
  {
    return FeatureSetPtr( (*fsp)->clone() );
  }
  else
  {
    throw insight::Exception
    (
      "Argument #"+lexical_cast<std::string>(id)+" is not a FeatureSet"
    );
  }
  return FeatureSetPtr();
}
BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, lookupFeatureSet_, lookupFeatureSet, 2);

arma::mat lookupMat(const FeatureSetParserArgList& fl, size_t id)
{
  if (id>=fl.size())
    throw insight::Exception
    (
      "Vector entry #"+lexical_cast<std::string>(id)
     +" is not present in list of size "+lexical_cast<std::string>(fl.size())
    );
  
  
  if (const VectorPtr* m = boost::get<VectorPtr>(&fl.at(id)))
  {
    return (*m)->value();
  }
  else
  {
    throw insight::Exception
    (
      "Argument #"+lexical_cast<std::string>(id)+" is not a vector/matrix"
    );
  }
  return arma::mat();
}

double lookupScalar(const FeatureSetParserArgList& fl, size_t id)
{
  if (id>=fl.size())
    throw insight::Exception
    (
      "scalar entry #"+lexical_cast<std::string>(id)
     +" is not present in list of size "+lexical_cast<std::string>(fl.size())
    );
  
  
  if (const ScalarPtr* m = boost::get<ScalarPtr>(&fl.at(id)))
  {
    return (*m)->value();
  }
  else
  {
    throw insight::Exception
    (
      "Argument #"+lexical_cast<std::string>(id)+" is not a scalar"
    );
  }
  return 0.0;
}
BOOST_PHOENIX_ADAPT_FUNCTION(arma::mat, vec3_, vec3, 3);

template <typename Iterator, typename Skipper = qi::space_type >
struct FeatureFilterExprParser
: qi::grammar<Iterator, FilterPtr(), Skipper>
{

public:
    qi::rule<Iterator, std::string(), Skipper> r_identifier;
    qi::rule<Iterator, FeatureSetPtr(), Skipper> r_featureset;
    qi::rule<Iterator, FilterPtr(), Skipper> r_filter_primary, r_filter_and, r_filter_or;
    qi::rule<Iterator, FilterPtr(), Skipper> r_filter;
    qi::rule<Iterator, FilterPtr(), Skipper> r_qty_comparison;
    qi::rule<Iterator, scalarQuantityComputer::Ptr(), Skipper> r_scalar_qty_expression, r_scalar_primary, r_scalar_term;
    qi::rule<Iterator, matQuantityComputer::Ptr(), Skipper> r_mat_qty_expression, r_mat_primary, r_mat_term;
    
    qi::rule<Iterator, FilterPtr(), Skipper> r_filter_functions;
    qi::rule<Iterator, scalarQuantityComputer::Ptr(), Skipper> r_scalar_qty_functions;
    qi::rule<Iterator, matQuantityComputer::Ptr(), Skipper> r_mat_qty_functions;
    
    FeatureSetParserArgList externalFeatureSets_;

    FeatureFilterExprParser
    (
      const FeatureSetParserArgList& extsets
    )
    : FeatureFilterExprParser::base_type(r_filter),
      externalFeatureSets_(extsets)
    {
	r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
	
	r_featureset = lexeme[ '%' >> qi::int_ ] 
	  [ qi::_val = phx::construct<FeatureSetPtr>(lookupFeatureSet_(externalFeatureSets_, qi::_1)) ];

        r_filter =  r_filter_or.alias();

	r_filter_or = 
	  ( r_filter_and >> lit("||") > r_filter_and ) 
	    [ _val = phx::construct<FilterPtr>(new_<OR>(*qi::_1, *qi::_2)) ] 
	  | r_filter_and
	    [ qi::_val = qi::_1 ]
	  ;
	
	r_filter_and =
	  ( r_filter_primary >> lit("&&") > r_filter_primary ) 
	    [ _val = phx::construct<FilterPtr>(new_<AND>(*qi::_1, *qi::_2)) ]
	  | r_filter_primary 
	    [ qi::_val = qi::_1 ]
	  ;
	  
	r_filter_primary =
	  ( r_filter_functions ) [ qi::_val = qi::_1 ]
	  |
	  ( lit("in") >> '(' > r_featureset > ')' ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<in>(*qi::_1)) ]
	  |
	  ( lit("maximal") >> '(' > r_scalar_qty_expression >> ( ( ',' > int_ ) | attr(0) ) >> ')' ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<maximal>(*qi::_1, qi::_2)) ]
	  |
	  ( lit("minimal") >> '(' > r_scalar_qty_expression >> ( ( ',' > int_ ) | attr(0) ) >> ')' ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<minimal>(*qi::_1, qi::_2)) ]
	  |
	  ( r_qty_comparison ) 
	    [ qi::_val = qi::_1 ]
	  |
	  ( '(' >> r_filter >> ')' ) 
	    [ qi::_val = qi::_1 ]
	  | 
	  ( '!' >> r_filter_primary ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<NOT>(*qi::_1)) ]
	  ;
	  
	r_qty_comparison = 
	  ( r_scalar_qty_expression >> lit("==") >> r_scalar_qty_expression ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<equal<double, double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_scalar_qty_expression >> '~' >> r_scalar_qty_expression >> ( ( '{' >> double_ >> '}' ) | attr(1e-2) ) ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<approximatelyEqual<double> >(*qi::_1, *qi::_2, qi::_3)) ]
	  |
	  ( r_scalar_qty_expression >> '>' >> r_scalar_qty_expression ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<greater<double, double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_scalar_qty_expression >> lit(">=") >> r_scalar_qty_expression ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<greaterequal<double, double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_scalar_qty_expression >> '<' >> r_scalar_qty_expression ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<less<double, double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_scalar_qty_expression >> lit("<=") >> r_scalar_qty_expression ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<lessequal<double, double> >(*qi::_1, *qi::_2)) ]
	  ;
	  
	r_scalar_qty_expression =
	  ( r_scalar_term >> '+' > r_scalar_term ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<added<double,double> >(*qi::_1, *qi::_2)) ]
	  | 
	  ( r_scalar_term >> '-' > r_scalar_term ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<subtracted<double,double> >(*qi::_1, *qi::_2)) ]
	  |
	  r_scalar_term 
	    [ qi::_val = qi::_1 ]
	  ;
	
	r_scalar_term =
	  r_scalar_primary 
	    [ qi::_val = qi::_1 ]
	  |
	  ( r_scalar_primary >> '*' >> r_scalar_primary ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<multiplied<double,double> >(*qi::_1, *qi::_2)) ]
	  | 
	  ( r_scalar_primary >> '/' >> r_scalar_primary ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<divided<double,double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_mat_primary >> '&' >> r_mat_primary ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<dotted<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  ;
	  
	r_scalar_primary =
	  double_ 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<constantQuantity<double> >(qi::_1)) ]
	  | r_scalar_qty_functions 
	    [ qi::_val = qi::_1 ]
	  | ( lit("mag") > '(' > r_scalar_qty_expression > ')' ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<mag<double> >(*qi::_1)) ]
	  | ( lit("dist") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<distance>(qi::_1, qi::_2)) ]
	  | ( lit("sqr") > '(' > r_scalar_qty_expression > ')' ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<sqr<double> >(*qi::_1)) ]
	  | ( lit("angle") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<angle<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  | ( lit("angleMag") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' ) 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<angleMag<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  |
	  lexeme[ lit("%d") >> qi::int_ ] 
	    [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>
	      ( new_<constantQuantity<double> >(phx::bind(&lookupScalar, externalFeatureSets_, qi::_1)) ) ]
	  | ( '(' >> r_scalar_qty_expression >> ')' ) 
	    [ qi::_val = qi::_1 ]
// 	  | ('-' >> r_scalar_primary) [ _val = -_1 ]
	  | ( r_mat_primary >> '.' >> 'x' ) 
	    [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compX<arma::mat> >(*qi::_1)) ]
	  | ( r_mat_primary >> '.' >> 'y' ) 
	    [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compY<arma::mat> >(*qi::_1)) ]
	  | ( r_mat_primary >> '.' >> 'z' ) 
	    [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compZ<arma::mat> >(*qi::_1)) ]
	  ;
	  
	r_mat_qty_expression =
	  r_mat_term 
	    [ qi::_val = qi::_1 ]
	  |
	  ( r_mat_term >> '+' >> r_mat_term ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<added<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  | 
	  ( r_mat_term >> '-' >> r_mat_term ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<subtracted<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  ;
	
	r_mat_term =
	(
	  r_mat_primary 
	    [ qi::_val = qi::_1 ]
	  |
	  ( r_mat_primary >> '*' >> r_mat_primary ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<multiplied<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  | 
	  ( r_mat_primary >> '/' >> r_mat_primary ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<divided<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	) 
	  ;
	  
	r_mat_primary =
	  ( '[' >> double_ >> ',' >> double_ >> ',' >> double_ >> ']' ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<constantQuantity<arma::mat> >(vec3_(qi::_1,qi::_2,qi::_3))) ]
	  | 
	  r_mat_qty_functions 
	    [ qi::_val = qi::_1 ]
	  | 
	  ( '(' >> r_mat_qty_expression >> ')' )
	    [ qi::_val = qi::_1 ]
// 	  | ('-' >> r_scalar_primary) [ qi::_val = -qi::_1 ]
	  |
	  lexeme[ lit("%m") >> qi::int_ ] 
	   [ qi::_val = phx::construct<matQuantityComputer::Ptr>
	      ( new_<constantQuantity<arma::mat> >(phx::bind(&lookupMat, externalFeatureSets_, qi::_1)) ) ];	  
	  ;

//       BOOST_SPIRIT_DEBUG_NODE(r_filter);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_or);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_and);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_primary);
//       
//       BOOST_SPIRIT_DEBUG_NODE(r_qty_comparison);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_qty_expression);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_term);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_primary);

      on_error<fail>(r_filter,
                       phx::ref(std::cout)
                       << "Error! Expecting "
                       << qi::_4
                       << " here: '"
                       << phx::construct<std::string>(qi::_3, qi::_2)
                       << "'\n"
                      );
    }  
    
    
};

template <typename Iterator >
struct VertexFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  VertexFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
      
      FeatureFilterExprParser<Iterator>::r_mat_qty_functions = 
        ( lit("loc") ) 
	  [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::vertexLocation>()) ]
      ;
    
  }
};


template <typename Iterator >
struct EdgeFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  EdgeFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
    FeatureFilterExprParser<Iterator>::r_filter_functions =
	( lit("isLine") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Line)) ]
	|
	( lit("isCircle") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Circle)) ]
	|
	( lit("isEllipse") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Ellipse)) ]
	|
	( lit("isHyperbola") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Hyperbola)) ]
	|
	( lit("isParabola") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Parabola)) ]
	|
	( lit("isBezierCurve") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_BezierCurve)) ]
	|
	( lit("isBSplineCurve") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_BSplineCurve)) ]
	|
	( lit("isOtherCurve") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_OtherCurve)) ]
	|
	( lit("isFaceBoundary") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<boundaryEdge>()) ]
	|
	( lit("isPartOfSolid") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<isPartOfSolidEdge>(*qi::_1)) ]
	|
	( lit("isCoincident") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<coincidentEdge>(*qi::_1)) ]
	|
	( lit("projectionIsCoincident") > '('
	  > FeatureFilterExprParser<Iterator>::r_featureset > ','
	  > FeatureFilterExprParser<Iterator>::r_mat_qty_expression > ',' // p0
	  > FeatureFilterExprParser<Iterator>::r_mat_qty_expression > ',' // n
	  > FeatureFilterExprParser<Iterator>::r_mat_qty_expression > ','    // up
	  > FeatureFilterExprParser<Iterator>::r_scalar_qty_expression > ')' // tol
	) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<coincidentProjectedEdge>(*qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
	  ;
	  
      FeatureFilterExprParser<Iterator>::r_scalar_qty_functions =
	( lit("len") ) 
	  [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<edgeLen>()) ]
	|
	( lit("radialLen") >> 
	    '(' >> FeatureFilterExprParser<Iterator>::r_mat_qty_expression >> //ax
	    ',' >> FeatureFilterExprParser<Iterator>::r_mat_qty_expression >>  //p0
	    ')' ) 
	  [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<edgeRadialLen>(qi::_1, qi::_2)) ]
      ;
      
      FeatureFilterExprParser<Iterator>::r_mat_qty_functions = 
//         ( lit("avgTangent") ) [ _val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::edgeAvgTangent>()) ]
//         |
        ( lit("CoG") ) 
	  [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::edgeCoG>()) ]
        |
        ( lit("start") ) 
	  [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::edgeStart>()) ]
        |
        ( lit("end") ) 
	  [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::edgeEnd>()) ]
      ;
    
  }
};


template <typename Iterator >
struct FaceFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  FaceFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
    FeatureFilterExprParser<Iterator>::r_filter_functions = 
	( lit("isPlane") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Plane)) ]
	|
	( lit("isCylinder") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Cylinder)) ]
	|
	( lit("isCone") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Cone)) ]
	|
	( lit("isSphere") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Sphere)) ]
	|
	( lit("isTorus") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Torus)) ]
	|
	( lit("isBezierSurface") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_BezierSurface)) ]
	|
	( lit("isBSplineSurface") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_BSplineSurface)) ]
	|
	( lit("isSurfaceOfRevolution") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_SurfaceOfRevolution)) ]
	|
	( lit("isSurfaceOfExtrusion") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_SurfaceOfExtrusion)) ]
	|
	( lit("isOffsetSurface") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_OffsetSurface)) ]
	|
	( lit("isOtherSurface") ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_OtherSurface)) ]
	|
	( lit("isPartOfSolid") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<isPartOfSolidFace>(*qi::_1)) ]
	|
	( lit("isCoincident") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<coincidentFace>(*qi::_1)) ]
	|
	( lit("adjacentToEdges") > '(' > FeatureFilterExprParser<Iterator>::r_featureset > ')' ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceAdjacentToEdges>(*qi::_1)) ]
	|
	( lit("adjacentToFaces") > '(' > FeatureFilterExprParser<Iterator>::r_featureset > ')' ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceAdjacentToFaces>(*qi::_1)) ]
      ;

      FeatureFilterExprParser<Iterator>::r_scalar_qty_functions =
	( lit("cylRadius") ) 
	  [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<cylRadius>()) ]
	|
	( lit("area") ) 
	  [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<faceArea>()) ]
      ;
      
      FeatureFilterExprParser<Iterator>::r_mat_qty_functions = 
        ( lit("normal") ) 
	  [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::faceNormalVector>()) ]
        |
        ( lit("CoG") ) 
	  [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::faceCoG>()) ]
      ;
    
  }
};


template <typename Iterator >
struct SolidFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  SolidFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
      
      FeatureFilterExprParser<Iterator>::r_mat_qty_functions = 
        ( lit("CoG") ) 
	  [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::solidCoG>()) ]
      ;
	  
      FeatureFilterExprParser<Iterator>::r_scalar_qty_functions = 
        ( lit("volume") ) 
	  [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<insight::cad::solidVolume>()) ]
      ;
    
  }
};

template<class Parser>
FilterPtr parseFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
try {
  Parser parser(refs);
//   skip_grammar<Iterator> skip;
  
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  
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
  
//   ModelStepsWriter writer;
//   parser.model_.modelstepSymbols.for_each(writer);

  if (first != last) // fail if we did not get a full match
  {
    int n=first-contents_raw.begin();
    throw insight::Exception( str(format("parsing of filtering expression '%s' failed after %d chars!")%contents_raw%n) );
    return FilterPtr();
  }
  
//   model = parser.model_;
  
  return result;
}
catch (insight::Exception e)
{
  std::cerr<<"Exception occurred: "<<e<<std::endl;
  throw e;
}

}

FilterPtr parseVertexFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
  return parseFilterExpr<VertexFeatureFilterExprParser<std::string::iterator> >(in, refs);
}

FilterPtr parseEdgeFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
  return parseFilterExpr<EdgeFeatureFilterExprParser<std::string::iterator> >(in, refs);
}

FilterPtr parseFaceFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
  return parseFilterExpr<FaceFeatureFilterExprParser<std::string::iterator> >(in, refs);
}

FilterPtr parseSolidFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
  return parseFilterExpr<SolidFeatureFilterExprParser<std::string::iterator> >(in, refs);
}


}
}
