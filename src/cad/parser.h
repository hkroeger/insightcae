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
                ;
        }

        qi::rule<Iterator> skip;

};

typedef double scalar;
typedef arma::mat vector;
typedef SolidModel solidmodel;
typedef std::pair<std::string, solidmodel > modelstep;
typedef std::vector<modelstep> model;



BOOST_PHOENIX_ADAPT_FUNCTION(vector, vec3_, vec3, 3);


template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct ISCADParser
  : qi::grammar<Iterator, Skipper>
{
//     std::map<std::string, double> scalarSymbols;
//     void insertScalarSymbol(std::pair<std::string, scalar>& p) { scalarSymbols[p.first] = p.second; }

    struct scalarSymbolTable : public qi::symbols<char, scalar> {} scalarSymbols;
    

    ISCADParser(model& m)
      : ISCADParser::base_type(r_model)
    {
      
	using namespace qi;
	using namespace phx;
	using namespace insight::cad;
      
        r_model =  *( r_assignment | r_modelstep );
	
	r_assignment = ( r_identifier >> '='  >> r_scalarExpression) [ phx::bind(scalarSymbols.add, _1, _2) ];
	
        r_modelstep  =  r_identifier >> "<<" >> r_solidmodel; //( (rentry>>qi::lit(';')) | rsubdict | (rraw>>qi::lit(';'))) ;
	
	r_solidmodel = 
	 ( lit("import") >> '(' >> r_path >> ')' ) // [ _val = import_(_1) ];
	 | r_solidmodel_by_primitive | r_solidmodel_operations;
	 
	r_solidmodel_by_primitive = 
	 // Primitives
	 ( ( lit("Sphere") >> '(' >> r_vector >> ',' >> r_scalarExpression >> ')' ) [ _val = construct<Sphere>(_1, _2) ] )
	 ;
	 
	r_solidmodel_operations = 
	 // Operations
	 ( ( r_solidmodel >> '|' >> r_solidmodel ) [ _val = construct<BooleanUnion>(_1, _2) ] )
	 ;
	 
	r_path = as_string [ 
                            lexeme [ "\"" >> *~char_("\"") >> "\"" ] 
                         ];
			 
	r_scalarExpression = 
	  scalarSymbols [ _val = _1 ]
	  | double_ [ _val = _1 ]
	  | ('(' >> r_scalarExpression >> ')') [_val=_1]
	  ;

	r_vector = ( "[" >> double_ >> "," >> double_ >> "," >> double_ >> "]" ) [ _val = vec3_(_1, _2, _3) ] ;

	r_identifier = alpha >> *(alnum | '_');
	 
	BOOST_SPIRIT_DEBUG_NODE(r_path);
	BOOST_SPIRIT_DEBUG_NODE(r_identifier);
	BOOST_SPIRIT_DEBUG_NODE(r_assignment);
	BOOST_SPIRIT_DEBUG_NODE(r_scalarExpression);
	BOOST_SPIRIT_DEBUG_NODE(r_vector);
// 	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel);
// 	BOOST_SPIRIT_DEBUG_NODE(r_modelstep);
// 	BOOST_SPIRIT_DEBUG_NODE(r_model);

    }
    
    qi::rule<Iterator, scalar(), Skipper> r_scalarExpression;
    qi::rule<Iterator, vector(), Skipper> r_vector;

    qi::rule<Iterator, Skipper> r_model;
    qi::rule<Iterator, Skipper> r_assignment;
    //qi::rule<Iterator, Skipper> r_var_decl;
    qi::rule<Iterator, modelstep(), Skipper> r_modelstep;
    qi::rule<Iterator, std::string()> r_identifier;
    qi::rule<Iterator, boost::filesystem::path()> r_path;
    qi::rule<Iterator, solidmodel(), Skipper> r_solidmodel;
    qi::rule<Iterator, solidmodel(), Skipper> r_solidmodel_by_primitive;
    qi::rule<Iterator, solidmodel(), Skipper> r_solidmodel_operations;
    
};

template <typename Parser, typename Result, typename Iterator>
bool parseISCADModel(Iterator first, Iterator last, Result& d)
{
  Parser parser(d);
  skip_grammar<Iterator> skip;
  
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
     
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
