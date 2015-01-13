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

#define BOOST_NO_CXX11_SCOPED_ENUMS

#include "boost/filesystem.hpp"
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "boost/assign.hpp"
#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>

#include "boost/format.hpp"
#include <boost/tokenizer.hpp>
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include <boost/algorithm/string.hpp>
#include "boost/date_time.hpp"

#include "boost/concept_check.hpp"
#include "boost/utility.hpp"

#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/ptr_container/ptr_map.hpp"

#include "boost/shared_ptr.hpp"

#include "boost/foreach.hpp"

#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/buffer_concepts.hpp>

#include <boost/fusion/include/std_pair.hpp>
#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#include "boost/variant.hpp"
#include "boost/variant/recursive_variant.hpp"
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/adapted.hpp>

#include "boost/thread.hpp"

#define BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_DEBUG

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

using namespace std;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;
using namespace boost;


template <typename Iterator, typename Skipper = qi::space_type >
qi::rule<Iterator, std::string(), Skipper> stringRule()
{
  return as_string[ lexeme [ '"' >> *~char_('"') >> '"' ] ];
}

struct ParserDataBase
{
  typedef boost::shared_ptr<ParserDataBase> Ptr;
};


struct DoubleParameterParser
{
  struct Data
  : public ParserDataBase
  {
    double value;
    std::string description;
    
    Data(double v, const std::string& d)
    : value(v), description(d) 
    {std::cout<<d<<std::endl;}
  };
  
  template <typename Iterator, typename Skipper = qi::space_type >
  inline static qi::rule<Iterator, ParserDataBase::Ptr(), Skipper> rule()
  {
    return 
      ( lit("double") >> qi::double_ >> stringRule<Iterator, Skipper>() ) 
      [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
      ;
  }
};

struct IntParameterParser
{
  struct Data
  : public ParserDataBase
  {
    int value;
    std::string description;
    
    Data(int v, const std::string& d)
    : value(v), description(d) 
    {std::cout<<d<<std::endl;}
  };
  
  template <typename Iterator, typename Skipper = qi::space_type >
  inline static qi::rule<Iterator, ParserDataBase::Ptr(), Skipper> rule()
  {
    return 
      ( lit("int") >> qi::int_ >> stringRule<Iterator,Skipper>() ) 
      [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
      ;
  }
};

typedef std::pair<std::string, ParserDataBase::Ptr> ParameterSetEntry;
typedef std::vector< ParameterSetEntry > ParameterSetData;

template <typename Iterator, typename Skipper = qi::space_type >
struct ParameterDescriptionLanguageParser
: qi::grammar<Iterator, ParameterSetData(), Skipper>
{

public:
     qi::rule<Iterator, std::string(), Skipper> r_identifier;
//     qi::rule<Iterator, FeatureSetPtr(), Skipper> r_featureset;
//     qi::rule<Iterator, FilterPtr(), Skipper> r_filter_primary, r_filter_and, r_filter_or;
    qi::rule<Iterator, ParameterSetEntry(), Skipper> r_parametersetentry;
    qi::rule<Iterator, ParameterSetData(), Skipper> r_parameterset;
//     qi::rule<Iterator, FilterPtr(), Skipper> r_qty_comparison;
//     qi::rule<Iterator, scalarQuantityComputer::Ptr(), Skipper> r_scalar_qty_expression, r_scalar_primary, r_scalar_term;
//     qi::rule<Iterator, matQuantityComputer::Ptr(), Skipper> r_mat_qty_expression, r_mat_primary, r_mat_term;
//     
//     qi::rule<Iterator, FilterPtr(), Skipper> r_filter_functions;
//     qi::rule<Iterator, scalarQuantityComputer::Ptr(), Skipper> r_scalar_qty_functions;
//     qi::rule<Iterator, matQuantityComputer::Ptr(), Skipper> r_mat_qty_functions;
    
//     FeatureSetList externalFeatureSets_;

    ParameterDescriptionLanguageParser()
    : ParameterDescriptionLanguageParser::base_type(r_parameterset)
    {
      r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
      r_parametersetentry = ( r_identifier >> (
	DoubleParameterParser::rule<Iterator, Skipper>()
	 /*|
	IntParameterParser::rule<Iterator, Skipper>() */
      )) [qi::_val=phx::construct<ParameterSetEntry>(qi::_1,qi::_2)];
      r_parameterset = *( r_parametersetentry );
      
      BOOST_SPIRIT_DEBUG_NODE(r_identifier);
      BOOST_SPIRIT_DEBUG_NODE(r_parametersetentry);
      BOOST_SPIRIT_DEBUG_NODE(r_parameterset);
      
// 	r_featureset = lexeme[ '%' >> qi::int_ ] [ qi::_val = phx::construct<FeatureSetPtr>(lookupFeatureSet_(externalFeatureSets_, qi::_1)) ];
// 
//         r_filter =  r_filter_or.alias();
// 
// 	r_filter_or = 
// 	  ( r_filter_and >> lit("||") > r_filter_and ) [ _val = phx::construct<FilterPtr>(new_<OR>(*qi::_1, *qi::_2)) ] 
// 	  | r_filter_and [ qi::_val = qi::_1 ]
// 	  ;
// 	
// 	r_filter_and =
// 	  ( r_filter_primary >> lit("&&") > r_filter_primary ) [ _val = phx::construct<FilterPtr>(new_<AND>(*qi::_1, *qi::_2)) ]
// 	  | r_filter_primary [ qi::_val = qi::_1 ]
// 	  ;
// 	  
// 	r_filter_primary =
// 	  ( r_filter_functions ) [ qi::_val = qi::_1 ]
// 	  |
// 	  ( lit("in") >> '(' > r_featureset > ')' ) 
// 	    [ qi::_val = phx::construct<FilterPtr>(new_<in>(*qi::_1)) ]
// 	  |
// 	  ( lit("maximal") >> '(' > r_scalar_qty_expression >> ( ( ',' > int_ ) | attr(0) ) >> ')' ) 
// 	    [ qi::_val = phx::construct<FilterPtr>(new_<maximal>(*qi::_1, qi::_2)) ]
// 	  |
// 	  ( lit("minimal") >> '(' > r_scalar_qty_expression >> ( ( ',' > int_ ) | attr(0) ) >> ')' ) 
// 	    [ qi::_val = phx::construct<FilterPtr>(new_<minimal>(*qi::_1, qi::_2)) ]
// 	  |
// 	  ( r_qty_comparison ) [ qi::_val = qi::_1 ]
// 	  |
// 	  ( '(' >> r_filter >> ')' ) [ qi::_val = qi::_1 ]
// 	  | 
// 	  ( '!' >> r_filter_primary ) [ qi::_val = phx::construct<FilterPtr>(new_<NOT>(*qi::_1)) ]
// 	  ;
// 	  
// 	r_qty_comparison = 
// 	  ( r_scalar_qty_expression >> lit("==") >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<equal<double, double> >(*qi::_1, *qi::_2)) ]
// 	  |
// 	  ( r_scalar_qty_expression >> '~' >> r_scalar_qty_expression >> ( ( '{' >> double_ >> '}' ) | attr(1e-2) ) ) 
// 	    [ qi::_val = phx::construct<FilterPtr>(new_<approximatelyEqual<double> >(*qi::_1, *qi::_2, qi::_3)) ]
// 	  |
// 	  ( r_scalar_qty_expression >> '>' >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<greater<double, double> >(*qi::_1, *qi::_2)) ]
// 	  |
// 	  ( r_scalar_qty_expression >> lit(">=") >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<greaterequal<double, double> >(*qi::_1, *qi::_2)) ]
// 	  |
// 	  ( r_scalar_qty_expression >> '<' >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<less<double, double> >(*qi::_1, *qi::_2)) ]
// 	  |
// 	  ( r_scalar_qty_expression >> lit("<=") >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<lessequal<double, double> >(*qi::_1, *qi::_2)) ]
// 	  ;
// 	  
// 	r_scalar_qty_expression =
// 	  ( r_scalar_term >> '+' > r_scalar_term ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<added<double,double> >(*qi::_1, *qi::_2)) ]
// 	  | 
// 	  ( r_scalar_term >> '-' > r_scalar_term ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<subtracted<double,double> >(*qi::_1, *qi::_2)) ]
// 	  |
// 	  r_scalar_term [ qi::_val = qi::_1 ]
// 	  ;
// 	
// 	r_scalar_term =
// 	  r_scalar_primary [ qi::_val = qi::_1 ]
// 	  |
// 	  ( r_scalar_primary >> '*' >> r_scalar_primary ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<multiplied<double,double> >(*qi::_1, *qi::_2)) ]
// 	  | 
// 	  ( r_scalar_primary >> '/' >> r_scalar_primary ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<divided<double,double> >(*qi::_1, *qi::_2)) ]
// 	  |
// 	  ( r_mat_primary >> '&' >> r_mat_primary ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<dotted<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
// 	  ;
// 	  
// 	r_scalar_primary =
// 	  /*lexeme[ model_->scalarSymbols >> !(alnum | '_') ] [ _val = _1 ]
// 	  |*/ double_ [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<constantQuantity<double> >(qi::_1)) ]
// 	  | r_scalar_qty_functions [ qi::_val = qi::_1 ]
// 	  | ( lit("mag") > '(' > r_scalar_qty_expression > ')' ) 
// 	   [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<mag<double> >(*qi::_1)) ]
// 	  | ( lit("sqr") > '(' > r_scalar_qty_expression > ')' ) 
// 	   [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<sqr<double> >(*qi::_1)) ]
// 	  | ( lit("angle") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' ) 
// 	   [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<angle<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
// 	  | ( lit("angleMag") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' ) 
// 	   [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<angleMag<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
// 	  | ( '(' >> r_scalar_qty_expression >> ')' ) [ qi::_val = qi::_1 ]
// // 	  | ('-' >> r_scalar_primary) [ _val = -_1 ]
// 	  | ( r_mat_primary >> '.' >> 'x' ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compX<arma::mat> >(*qi::_1)) ]
// 	  | ( r_mat_primary >> '.' >> 'y' ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compY<arma::mat> >(*qi::_1)) ]
// 	  | ( r_mat_primary >> '.' >> 'z' ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compZ<arma::mat> >(*qi::_1)) ]
// 	  ;
// 	  
// 	r_mat_qty_expression =
// 	  r_mat_term [ qi::_val = qi::_1 ]
// 	  |
// 	  ( r_mat_term >> '+' >> r_mat_term ) 
// 	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<added<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
// 	  | 
// 	  ( r_mat_term >> '-' >> r_mat_term ) 
// 	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<subtracted<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
// 	  ;
// 	
// 	r_mat_term =
// 	(
// 	  r_mat_primary [ qi::_val = qi::_1 ]
// 	  |
// 	  ( r_mat_primary >> '*' >> r_mat_primary ) 
// 	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<multiplied<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
// 	  | 
// 	  ( r_mat_primary >> '/' >> r_mat_primary ) 
// 	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<divided<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
// 	) /*| (
// 	  r_vector_primary >> '&' >> r_vector_primary
// 	) [_val = dot_(_1, _2) ]*/
// 	  ;
// 	  
// 	r_mat_primary =
// 	  /*lexeme[ model_->scalarSymbols >> !(alnum | '_') ] [ _val = _1 ]
// 	  |*/ 
// 	  ( '[' >> double_ >> ',' >> double_ >> ',' >> double_ >> ']' ) 
// 	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<constantQuantity<arma::mat> >(vec3_(qi::_1,qi::_2,qi::_3))) ]
// 	  | 
// 	  r_mat_qty_functions [ qi::_val = qi::_1 ]
// 	  | 
// 	  ( '(' >> r_mat_qty_expression >> ')' ) [ qi::_val = qi::_1 ]
// // 	  | ('-' >> r_scalar_primary) [ qi::_val = -qi::_1 ]
// 	  ;

//       BOOST_SPIRIT_DEBUG_NODE(r_filter);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_or);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_and);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_primary);
//       
//       BOOST_SPIRIT_DEBUG_NODE(r_qty_comparison);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_qty_expression);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_term);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_primary);

      on_error<fail>(r_parameterset,
                       phx::ref(std::cout)
                       << "Error! Expecting "
                       << qi::_4
                       << " here: '"
                       << phx::construct<std::string>(qi::_3, qi::_2)
                       << "'\n"
                      );
    }
    
};


int main(int argc, char *argv[])
{
  std::ifstream in(argv[1]);

  ParameterDescriptionLanguageParser<std::string::iterator> parser;
  //   skip_grammar<Iterator> skip;
    
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  cout<<contents_raw<<endl;
  
  std::string::iterator first=contents_raw.begin();
  std::string::iterator last=contents_raw.end();
  
  ParameterSetData result;
  bool r = qi::phrase_parse
  (
      first, 
      last,
      parser,
      qi::space,
      result
  );
  
  return 0;
}