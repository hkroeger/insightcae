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


#include "featurefilter.h"

#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/phoenix.hpp>


#include "cadparameters/constantscalar.h"
#include "cadparameters/constantvector.h"

#include "quantitycomputers/constantquantity.h"
#include "quantitycomputers/distance.h"

#include "featurefilters/booleanfilters.h"
#include "featurefilters/in.h"
#include "featurefilters/maximal.h"
#include "featurefilters/minimal.h"
#include "featurefilters/relationfilters.h"
#include "featurefilters/approximatelyequal.h"


namespace insight 
{
namespace cad 
{




namespace phx   = boost::phoenix;



using namespace qi;
using namespace phx;
using namespace insight::cad;




FeatureSetPtr lookupFeatureSet(const FeatureSetParserArgList& fl, size_t id)
{
  if (id>=fl.size())
    throw insight::Exception
    (
      "Feature set #%d is not present in list of size %d",
          id, fl.size()
    );
  
  
  if (const FeatureSetPtr* fsp = boost::get<FeatureSetPtr>(&fl.at(id)))
  {
    return FeatureSetPtr( (*fsp)->clone() );
  }
  else
  {
    throw insight::Exception
    (
      "Argument #%d is not a FeatureSet", id
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
      "Vector entry #%d is not present in list of size %d",
          id, fl.size()
    );
  
  
  if (const VectorPtr* m = boost::get<VectorPtr>(&fl.at(id)))
  {
    return (*m)->value();
  }
  else
  {
    throw insight::Exception
    (
      "Argument #%d is not a vector/matrix",
          id
    );
  }
  return arma::mat();
}




double lookupScalar(const FeatureSetParserArgList& fl, size_t id)
{
  if (id>=fl.size())
    throw insight::Exception
    (
      "scalar entry #%d is not present in list of size %d",
          id, fl.size()
    );
  
  
  if (const ScalarPtr* m = boost::get<ScalarPtr>(&fl.at(id)))
  {
    return (*m)->value();
  }
  else
  {
    throw insight::Exception
    (
      "Argument #%d is not a scalar",
          id
    );
  }
  return 0.0;
}
BOOST_PHOENIX_ADAPT_FUNCTION(arma::mat, vec3_, vec3, 3);




FeatureFilterExprParser::FeatureFilterExprParser(
        const FeatureSetParserArgList& extsets )
    : FeatureFilterExprParser::base_type(r_filter),
    externalFeatureSets_(extsets)
{
    r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];

    r_featureset = lexeme[ '%' >> qi::int_ ]
                         [ qi::_val = phx::construct<FeatureSetPtr>(lookupFeatureSet_(externalFeatureSets_, qi::_1)) ];

    r_filter =  r_filter_or.alias();

    r_filter_or =
        ( r_filter_and >> lit("||") > r_filter_and )
            [ _val = phx::bind(
                &OR::create<FilterArg,FilterArg>, qi::_1, qi::_2) ]
        | r_filter_and
            [ qi::_val = qi::_1 ]
        ;

    r_filter_and =
        ( r_filter_primary >> lit("&&") > r_filter_primary )
            [ _val = phx::bind(
                &AND::create<FilterArg,FilterArg>, qi::_1, qi::_2) ]
        | r_filter_primary
            [ qi::_val = qi::_1 ]
        ;

    r_filter_primary =
        ( r_filter_functions ) [ qi::_val = qi::_1 ]
        |
        ( lit("in") >> '(' > r_featureset > ')' )
            [ qi::_val = phx::bind(&in::create<FeatureSetPtr>, qi::_1) ]
        |
        ( lit("maximal") >> '(' > r_scalar_qty_expression
         >> ( ( ',' > int_ ) | attr(0) )
         >> ( ( lit("through") > int_ ) | attr(-1) )
         >> ')' )
            [ qi::_val = phx::bind(
                &maximal::create<ScalarArg,int,int>,
                                qi::_1, qi::_2, qi::_3) ]
        |
        ( lit("minimal") >> '(' > r_scalar_qty_expression
         >> ( ( ',' > int_ ) | attr(0) )
         >> ( ( lit("through") > int_ ) | attr(-1) )
         >> ')' )
            [ qi::_val = phx::bind(
                &minimal::create<ScalarArg,int,int>,
                                qi::_1, qi::_2, qi::_3) ]
        |
        ( r_qty_comparison )
            [ qi::_val = qi::_1 ]
        |
        ( '(' >> r_filter >> ')' )
            [ qi::_val = qi::_1 ]
        |
        ( '!' >> r_filter_primary )
            [ qi::_val = phx::bind(&NOT::create<FilterArg>, qi::_1) ]
        ;

    r_qty_comparison =
        ( r_scalar_qty_expression >> lit("==") >> r_scalar_qty_expression )
            [ qi::_val = phx::bind(
                &equal<double, double>::create<QCArg<double>,QCArg<double> >,
                    qi::_1, qi::_2 ) ]
        |
        ( r_scalar_qty_expression >> '~' >> r_scalar_qty_expression >> ( ( '{' >> double_ >> '}' ) | attr(1e-2) ) )
            [ qi::_val = phx::bind(
                &approximatelyEqual<double>::create<QCArg<double>,QCArg<double>,double >,
                                qi::_1, qi::_2, qi::_3) ]
        |
        ( r_scalar_qty_expression >> '>' >> r_scalar_qty_expression )
            [ qi::_val = phx::bind(
                &greater<double, double>::create<QCArg<double>,QCArg<double> >,
                                qi::_1, qi::_2) ]
        |
        ( r_scalar_qty_expression >> lit(">=") >> r_scalar_qty_expression )
            [ qi::_val = phx::bind(
                &greaterequal<double, double>::create<QCArg<double>,QCArg<double> >,
                                qi::_1, qi::_2) ]
        |
        ( r_scalar_qty_expression >> '<' >> r_scalar_qty_expression )
            [ qi::_val = phx::bind(
                &less<double, double>::create<QCArg<double>,QCArg<double> >,
                                qi::_1, qi::_2) ]
        |
        ( r_scalar_qty_expression >> lit("<=") >> r_scalar_qty_expression )
            [ qi::_val = phx::bind(
                &lessequal<double, double>::create<QCArg<double>,QCArg<double> >,
                                qi::_1, qi::_2) ]
        ;

    r_scalar_qty_expression =
        r_scalar_term [qi::_val=qi::_1] >>
        *(
            ( '+' >> r_scalar_term )
                [ qi::_val = phx::bind(&added<double,double>::create, qi::_val, qi::_1) ]
            |
            ( '-' >> r_scalar_term )
                [ qi::_val = phx::bind(&subtracted<double,double>::create, qi::_val, qi::_1) ]
            )
        ;

    r_scalar_term =
        (
            r_scalar_primary [ qi::_val = qi::_1 ]
            >> *(
                ( '*' >> r_scalar_primary )
                    [ qi::_val = phx::bind(&multiplied<double,double>::create, qi::_val, qi::_1) ]
                |
                ( '/' >> r_scalar_primary )
                    [ qi::_val = phx::bind(&divided<double,double>::create, qi::_val, qi::_1) ]
                )
            )
        |
        ( r_mat_primary >> '&' >> r_mat_primary )
            [ qi::_val = phx::bind(dotted<arma::mat,arma::mat>::create, qi::_1, qi::_2) ]
        ;

    r_scalar_primary =
        double_
            [ qi::_val = phx::bind(&constantQuantity<double>::create<const double&>, qi::_1) ]
        | r_scalar_qty_functions
            [ qi::_val = qi::_1 ]
        | ( lit("sin") > '(' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(&sin<double>::create, qi::_1) ]
        | ( lit("cos") > '(' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(&cos<double>::create, qi::_1) ]
        | ( lit("tan") > '(' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(&tan<double>::create, qi::_1) ]
        | ( lit("asin") > '(' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(&asin<double>::create, qi::_1) ]
        | ( lit("acos") > '(' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(&acos<double>::create, qi::_1) ]
        | ( lit("atan") > '(' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(&atan<double>::create, qi::_1) ]
        | ( lit("mag") > '(' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(&mag<double>::create, qi::_1) ]
        | ( lit("dist") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' )
            [ qi::_val = phx::bind(&distance::create, qi::_1, qi::_2) ]
        | ( lit("sqrt") > '(' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(&sqrt<double>::create, qi::_1) ]
        | ( lit("sqr") > '(' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(&sqr<double>::create, qi::_1) ]
        | ( lit("pow") > '(' > r_scalar_qty_expression > ',' > r_scalar_qty_expression > ')' )
            [ qi::_val = phx::bind(powed<double,double>::create, qi::_1, qi::_2) ]
        | ( lit("angleMag") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' ) // before "angle"!
            [ qi::_val = phx::bind(angleMag<arma::mat,arma::mat>::create, qi::_1, qi::_2) ]
        | ( lit("angle") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' )
            [ qi::_val = phx::bind(angle<arma::mat,arma::mat>::create, qi::_1, qi::_2) ]
        |
        lexeme[ lit("%d") >> qi::int_ ]
              [ qi::_val = phx::bind
               ( &constantQuantity<double>::create<const double&>,
                    phx::bind(&lookupScalar, externalFeatureSets_, qi::_1) ) ]
        | ( '(' >> r_scalar_qty_expression >> ')' )
            [ qi::_val = qi::_1 ]
        // 	  | ('-' >> r_scalar_primary) [ _val = -_1 ]
        | ( r_mat_qty_expression >> '.' >> 'x' )
            [ _val = phx::bind(&compX<arma::mat>::create, qi::_1) ]
        | ( r_mat_qty_expression >> '.' >> 'y' )
            [ _val = phx::bind(&compY<arma::mat>::create, qi::_1) ]
        | ( r_mat_qty_expression >> '.' >> 'z' )
            [ _val = phx::bind(&compZ<arma::mat>::create, qi::_1) ]
        ;

    r_mat_qty_expression =
        r_mat_term [ qi::_val = qi::_1 ]
        >>
        *(
            ( /*r_mat_term >>*/ '+' >> r_mat_term )
                [ qi::_val = phx::bind(&added<arma::mat,arma::mat>::create, qi::_val, qi::_1) ]
            |
            ( /*r_mat_term >>*/ '-' >> r_mat_term )
                [ qi::_val = phx::bind(&subtracted<arma::mat,arma::mat>::create, qi::_val, qi::_1) ]
            )
        ;

    r_mat_term =
        (
            r_mat_primary
                [ qi::_val = qi::_1 ]
            >> *(
                ( /*r_mat_primary >>*/ '*' >> r_mat_primary )
                    [ qi::_val = phx::bind(&multiplied<arma::mat,arma::mat>::create,
                                                qi::_val, qi::_1) ]
                |
                ( /*r_mat_primary >> */'/' >> r_mat_primary )
                    [ qi::_val = phx::bind(&divided<arma::mat,arma::mat>::create,
                                                qi::_val, qi::_1) ]
                )
            )
        ;

    r_mat_primary =
        ( '[' >> double_ >> ',' >> double_ >> ',' >> double_ >> ']' )
            [ qi::_val = phx::bind(&constantQuantity<arma::mat>::create<const arma::mat&>,
                             vec3_(qi::_1,qi::_2,qi::_3) ) ]
        |
        r_mat_qty_functions
            [ qi::_val = qi::_1 ]
        |
        ( '(' >> r_mat_qty_expression >> ')' )
            [ qi::_val = qi::_1 ]
        |
        lexeme[ lit("%m") > qi::int_ ]
              [ qi::_val = phx::bind
               ( &constantQuantity<arma::mat>::create< const arma::mat&>,
                    phx::bind(&lookupMat, externalFeatureSets_, qi::_1) ) ];
    ;


    on_error<fail>(r_filter,
                   phx::bind(&featureFilterParseError, qi::_1, qi::_2, qi::_3, qi::_4));

}



void FeatureFilterExprParser::featureFilterParseError(
    FeatureFilterIter /*first*/,
    FeatureFilterIter last,
    FeatureFilterIter error_pos,
    const boost::spirit::info& what)
{
    std::cout << "Error! Expecting " << what
              << " here: '" << std::string(error_pos, last) << "'\n";
}


}
}
