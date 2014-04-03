/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef INSIGHT_CAD_PARSER_H
#define INSIGHT_CAD_PARSER_H

#define BOOST_SPIRIT_DEBUG
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include "solidmodel.h"

#include <boost/fusion/include/std_pair.hpp>
#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#include "boost/variant.hpp"
#include "boost/ptr_container/ptr_map.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>

namespace insight {
namespace cad {
namespace parser {

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

template <typename Iterator>
struct skip_grammar : public qi::grammar<Iterator>
{
        skip_grammar() : skip_grammar::base_type(skip, "PL/0")
        {
            skip
                =   boost::spirit::ascii::space
                | repo::confix("/*", "*/")[*(qi::char_ - "*/")]
                | repo::confix("//", qi::eol)[*(qi::char_ - qi::eol)]
                | repo::confix("#", qi::eol)[*(qi::char_ - qi::eol)]
                ;
        }

        qi::rule<Iterator> skip;

};

typedef double scalar;
typedef arma::mat vector;
typedef SolidModel solidmodel;
typedef std::pair<std::string, solidmodel > modelstep;
typedef std::vector<modelstep> model;


double dot(const vector& v1, const vector& v2)
{
  return arma::as_scalar(arma::dot(v1,v2));
}

BOOST_PHOENIX_ADAPT_FUNCTION(vector, vec3_, vec3, 3);
BOOST_PHOENIX_ADAPT_FUNCTION(vector, cross_, cross, 2);
BOOST_PHOENIX_ADAPT_FUNCTION(vector, trans_, arma::trans, 1);
BOOST_PHOENIX_ADAPT_FUNCTION(double, dot_, dot, 2);


template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct ISCADParser
  : qi::grammar<Iterator, Skipper>
{

    struct scalarSymbolTable : public qi::symbols<char, scalar> {} scalarSymbols;
    struct vectorSymbolTable : public qi::symbols<char, vector> {} vectorSymbols;
    struct modelstepSymbolTable : public qi::symbols<char, solidmodel> {} modelstepSymbols;
    

    ISCADParser()
      : ISCADParser::base_type(r_model)
    {
      
	using namespace qi;
	using namespace phx;
	using namespace insight::cad;

	boost::spirit::classic::distinct_directive<> keyword_d("a-zA-Z0-9_");
	
        r_model =  *( r_assignment | r_modelstep );
	
	r_assignment = 
	  ( r_identifier >> '='  >> r_scalarExpression >> ';') [ phx::bind(scalarSymbols.add, _1, _2) ]
	  |
	  ( r_identifier >> '='  >> r_vectorExpression >> ';') [ phx::bind(vectorSymbols.add, _1, _2) ]
	  ;
	
        r_modelstep  =  ( r_identifier >> ':' > r_solidmodel_expression >> ';' ) [ phx::bind(modelstepSymbols.add, _1, _2) ];
	
	
	r_solidmodel_expression =
	 r_solidmodel_term [_val=_1 ]
	 >> *( '-' >> r_solidmodel_term [_val=construct<BooleanSubtract>(_val, _1)] )
	 ;
	
	r_solidmodel_term =
	 r_solidmodel_primary [_val=_1 ]
	 >> *( '|' >> r_solidmodel_primary [_val=construct<BooleanUnion>(_val, _1)] )
	 ;
	
	r_solidmodel_primary = 
	 modelstepSymbols [ _val = _1 ]
	 | '(' >> r_solidmodel_expression [_val=_1] >> ')'
         | ( lit("import") > '(' >> r_path >> ')' ) [ _val = construct<SolidModel>(_1) ]
	 // Primitives
	 | ( lit("Sphere") > '(' >> r_vectorExpression >> ',' >> r_scalarExpression >> ')' ) 
	      [ _val = construct<Sphere>(_1, _2) ]
	 | ( lit("Cylinder") > '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_scalarExpression >> ')' ) 
	      [ _val = construct<Cylinder>(_1, _2, _3) ]
	 | ( lit("Box") > '(' >> r_vectorExpression >> ',' >> r_vectorExpression 
			>> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
	      [ _val = construct<Box>(_1, _2, _3, _4) ]
	 ;
	 
	 
	r_path = as_string[ 
                            lexeme [ "\"" >> *~char_("\"") >> "\"" ] 
                         ];
			 
	r_scalarExpression = 
	  r_scalar_term [_val =_1]  >> *(
	    ( '+' >> r_scalar_term [_val+=_1] )
	  | ( '-' >> r_scalar_term [_val-=_1] )
	  )
	  ;
	
	r_scalar_term =
	(
	  r_scalar_primary [_val=_1] >> *(
	    ( '*' >> r_scalar_primary [ _val*=_1 ] )
	  | ( '/' >> r_scalar_primary [ _val/=_1 ] )
	  )
	) | (
	  r_vector_primary >> '&' >> r_vector_primary
	) [_val = dot_(_1, _2) ]
	  ;
	  
	r_scalar_primary =
	  scalarSymbols [ _val = _1 ]
	  | double_ [ _val = _1 ]
	  | ('(' >> r_scalarExpression >> ')') [_val=_1]
	  ;


	r_vectorExpression =
	  r_vector_term [_val =_1]  >> *(
	    ( '+' >> r_vector_term [_val+=_1] )
	  | ( '-' >> r_vector_term [_val-=_1] )
	  )
	  ;
	
	r_vector_term =
	(
	  r_vector_primary [_val=_1] >> *(
	    ( '*' >> r_scalar_term [ _val*=_1 ] )
	  | ( '/' >> r_scalar_term [ _val/=_1 ] )
	  | ( '^' >> r_vector_primary [ _val=cross_(_val, _1) ] )
	  )
	) | (
	  r_scalar_primary >> '*' >> r_vector_term
	) [_val=_1*_2]
	;
	  
	r_vector_primary =
	  lexeme[ vectorSymbols >> !(alnum | '_') ] [ _val = _1 ]
	  //vectorSymbols [ _val = _1 ]
	  | ( "[" >> r_scalarExpression >> "," >> r_scalarExpression >> "," >> r_scalarExpression >> "]" ) [ _val = vec3_(_1, _2, _3) ] 
	  //| ( r_vectorExpression >> '\'') [ _val = trans_(_1) ]
	  | ( '(' >> r_vectorExpression >> ')' ) [_val=_1]
	  ;

	r_identifier = lexeme[ alpha >> *(alnum | '_') >> !(alnum | '_') ];
	 
	BOOST_SPIRIT_DEBUG_NODE(r_path);
	BOOST_SPIRIT_DEBUG_NODE(r_identifier);
	BOOST_SPIRIT_DEBUG_NODE(r_assignment);
	BOOST_SPIRIT_DEBUG_NODE(r_scalar_primary);
	BOOST_SPIRIT_DEBUG_NODE(r_scalar_term);
	BOOST_SPIRIT_DEBUG_NODE(r_scalarExpression);
	BOOST_SPIRIT_DEBUG_NODE(r_vector_primary);
	BOOST_SPIRIT_DEBUG_NODE(r_vector_term);
	BOOST_SPIRIT_DEBUG_NODE(r_vectorExpression);
// 	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel_expression);
//	BOOST_SPIRIT_DEBUG_NODE(r_modelstep);
// 	BOOST_SPIRIT_DEBUG_NODE(r_model);

    }
    
    qi::rule<Iterator, scalar(), Skipper> r_scalar_primary, r_scalar_term, r_scalarExpression;
    qi::rule<Iterator, vector(), Skipper> r_vector_primary, r_vector_term, r_vectorExpression;

    qi::rule<Iterator, Skipper> r_model;
    qi::rule<Iterator, Skipper> r_assignment;
    qi::rule<Iterator, modelstep(), Skipper> r_modelstep;
    qi::rule<Iterator, std::string()> r_identifier;
    qi::rule<Iterator, boost::filesystem::path()> r_path;
    qi::rule<Iterator, solidmodel(), Skipper> r_solidmodel_primary, r_solidmodel_term, r_solidmodel_expression;
    
};

template<typename T>
struct ModelStepsWriter
: public std::map<std::string, T>
{
    void operator() (std::basic_string<char> s, T ct)
    {
      cout<<s<<endl<<ct<<endl;
      //(*this)[s]=ct;
      ct.saveAs(s+".brep");
    }
};

template <typename Parser, typename Result, typename Iterator>
bool parseISCADModel(Iterator first, Iterator last, Result& d)
{
  Parser parser;
  skip_grammar<Iterator> skip;
  
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
  
  ModelStepsWriter<SolidModel> writer;
  parser.modelstepSymbols.for_each(writer);

  if (first != last) // fail if we did not get a full match
      return false;
  
  return r;
}

}

bool parseISCADModelStream(std::istream& in, parser::model& m);

}
}

#undef BOOST_SPIRIT_DEBUG

#endif // INSIGHT_CAD_PARSER_H
