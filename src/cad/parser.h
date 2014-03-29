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

/*
solidmodel import(const boost::filesystem::path& filepath);

// primitives
solidmodel cylinder(const vector& p1, const vector& p2, scalar D);
solidmodel box
(
  const vector& p0, 
  const vector& L1, 
  const vector& L2, 
  const vector& L3
);
solidmodel sphere(const vector& p, scalar D);

// operations
solidmodel booleanUnion(const aolidmodel& m1, const solidmodel& m2);
*/

BOOST_PHOENIX_ADAPT_FUNCTION(vector, vec3_, vec3, 3);
//BOOST_PHOENIX_ADAPT_CALLABLE(import_, SolidModel, 1);

template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct ISCADParser
  : qi::grammar<Iterator, model(), Skipper>
{
    ISCADParser()
      : ISCADParser::base_type(r_model)
    {
	using namespace qi;
	using namespace phx;
	using namespace insight::cad;
      
        r_model =  *( r_modelstep );
	
        r_modelstep  =  r_identifier >> lit('=') >> r_solidmodel; //( (rentry>>qi::lit(';')) | rsubdict | (rraw>>qi::lit(';'))) ;
	
	r_solidmodel = 
	 ( lit("import") >> '(' >> r_path >> ')' ) // [ _val = import_(_1) ];
	 | r_solidmodel_by_primitive | r_solidmodel_operations;
	 
	r_solidmodel_by_primitive = 
	 // Primitives
	 ( ( lit("Sphere") >> '(' >> r_vector >> ',' >> r_scalar >> ')' ) [ _val = construct<Sphere>(_1, _2) ] )
	 ;
	 
	r_solidmodel_operations = 
	 // Operations
	 ( ( r_solidmodel >> '|' >> r_solidmodel ) [ _val = construct<BooleanUnion>(_1, _2) ] )
	 ;
	 
	r_path = as_string [ 
                            lexeme [ "\"" >> *~char_("\"") >> "\"" ] 
                         ];
			 
	r_scalar = qi::double_;

	r_vector = (lit("[") >> qi::double_ >> lit(",") >> qi::double_ >> lit(",") >> qi::double_ >> lit("]") ) [ _val = vec3_(_1, _2, _3) ] ;

	r_identifier = qi::alpha >> *qi::alnum;
	 
	/*
        ridentifier  =  +(~qi::char_(" \"\\/;{}()\n"));
	
	rstring = '"' >> *(~qi::char_('"')) >> '"';
	
	rraw = (~qi::char_("\"{}();") >> *(~qi::char_(';')) )|qi::string("");
	
	rentry = (qi::int_ | qi::double_ | rdimensionedData | rlist | rstring | ridentifier );
	
	rdimensionedData = ridentifier >> qi::lit('[') >> qi::repeat(7)[qi::int_] >> qi::lit(']') >> rentry;
	
        rsubdict = qi::lit('{')
	      >> *(rpair) >> qi::lit('}');
	      
        rlist = qi::lit('(')
	      >> *(rentry) >> qi::lit(')');
*/
	BOOST_SPIRIT_DEBUG_NODE(r_path);
	BOOST_SPIRIT_DEBUG_NODE(r_identifier);
	BOOST_SPIRIT_DEBUG_NODE(r_scalar);
	BOOST_SPIRIT_DEBUG_NODE(r_vector);
// 	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel);
// 	BOOST_SPIRIT_DEBUG_NODE(r_modelstep);
// 	BOOST_SPIRIT_DEBUG_NODE(r_model);
/*	      
	BOOST_SPIRIT_DEBUG_NODE(ridentifier);
	BOOST_SPIRIT_DEBUG_NODE(rstring);
	BOOST_SPIRIT_DEBUG_NODE(rraw);
	*/
	//BOOST_SPIRIT_DEBUG_NODE(rentry);
	//BOOST_SPIRIT_DEBUG_NODE(rsubdict);
	//BOOST_SPIRIT_DEBUG_NODE(rlist);

    }
    
    qi::rule<Iterator, scalar(), Skipper> r_scalar;
    qi::rule<Iterator, vector(), Skipper> r_vector;

    qi::rule<Iterator, model(), Skipper> r_model;
    qi::rule<Iterator, modelstep(), Skipper> r_modelstep;
    qi::rule<Iterator, std::string()> r_identifier;
    qi::rule<Iterator, boost::filesystem::path()> r_path;
    qi::rule<Iterator, solidmodel(), Skipper> r_solidmodel;
    qi::rule<Iterator, solidmodel(), Skipper> r_solidmodel_by_primitive;
    qi::rule<Iterator, solidmodel(), Skipper> r_solidmodel_operations;
/*
    qi::rule<Iterator, std::string()> rstring;
    qi::rule<Iterator, std::string()> rraw;
    qi::rule<Iterator, OFDictData::data(), Skipper> rentry;
    qi::rule<Iterator, OFDictData::dimensionedData(), Skipper> rdimensionedData;
    qi::rule<Iterator, OFDictData::dict(), Skipper> rsubdict;
    qi::rule<Iterator, OFDictData::list(), Skipper> rlist;
    */
    
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
      skip,
      d
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
