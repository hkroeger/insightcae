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

/* Basic rules */
template <typename Iterator, typename Skipper = qi::space_type >
qi::rule<Iterator, std::string(), Skipper> stringRule()
{
  return as_string[ lexeme [ "\"" >> *~char_("\"") >> "\"" ] ];
}

template <typename Iterator, typename Skipper = qi::space_type >
qi::rule<Iterator, std::string(), Skipper> identifierRule()
{
  return lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
}


/* Basic data structures */
class ParserDataBase
{
public:
  typedef boost::shared_ptr<ParserDataBase> Ptr;
  
  virtual void writeCppHeader(std::ostream& os, const std::string& name) const =0;
};

typedef std::pair<std::string, ParserDataBase::Ptr> ParameterSetEntry;
typedef std::vector< ParameterSetEntry > ParameterSetData;


template <typename Iterator, typename Skipper = qi::space_type >
struct ParameterDescriptionLanguageParserRuleset
{
  typedef qi::rule<Iterator, ParserDataBase::Ptr(), Skipper> ParameterDataRule;

  qi::rule<Iterator, std::string(), Skipper> r_identifier;
  qi::rule<Iterator, ParameterSetData(), Skipper> r_parameterset;
  qi::rule<Iterator, ParameterSetEntry(), Skipper> r_parametersetentry;
  
  std::vector< ParameterDataRule > parameterDataRules;
  qi::rule<Iterator, ParserDataBase::Ptr(), Skipper> r_parameterdata;
  
  void init()
  {
    r_parameterdata=parameterDataRules[0].copy();
    for (int i=1; i<parameterDataRules.size(); i++)
      r_parameterdata = r_parameterdata.copy() | parameterDataRules[i];
    
    r_identifier = identifierRule<Iterator, Skipper>();
    
    r_parametersetentry = r_identifier >> r_parameterdata;
    r_parameterset = *( r_parametersetentry );
    
    BOOST_SPIRIT_DEBUG_NODE(r_identifier);
    BOOST_SPIRIT_DEBUG_NODE(r_parameterdata);
    BOOST_SPIRIT_DEBUG_NODE(r_parameterset);
    BOOST_SPIRIT_DEBUG_NODE(r_parametersetentry);
  }
  
};

/* specialized data structures */
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
    
    virtual void writeCppHeader(std::ostream& os, const std::string& name) const
    {
      os<<"double "<<name<<";"<<endl;
    }
  };
  
  template <typename Iterator, typename Skipper = qi::space_type >
  inline static void insertrule(ParameterDescriptionLanguageParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.push_back( 
      ( lit("double") >> qi::double_ >> stringRule<Iterator, Skipper>().copy() ) 
      [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
    );
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

    virtual void writeCppHeader(std::ostream& os, const std::string& name) const
    {
      os<<"int "<<name<<";"<<endl;
    }
  };
  
  template <typename Iterator, typename Skipper = qi::space_type >
  inline static void insertrule(ParameterDescriptionLanguageParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.push_back(
      ( lit("int") >> qi::int_ >> stringRule<Iterator,Skipper>().copy() ) 
      [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
    );
  }
};

struct SubsetParameterParser
{
  struct Data
  : public ParserDataBase
  {
    ParameterSetData value;
    std::string description;
    
    Data(const ParameterSetData& v, const std::string& d)
    : value(v), description(d) 
    {std::cout<<d<<std::endl;}

    virtual void writeCppHeader(std::ostream& os, const std::string& name) const
    {
      os<<"struct {"<<endl;
      BOOST_FOREACH(const ParameterSetEntry& pe, value)
      {
	pe.second->writeCppHeader(cout, pe.first);
      }
      os<<"} "<<name<<";"<<endl;
    }
  };
  
  template <typename Iterator, typename Skipper = qi::space_type >
  inline static void insertrule(ParameterDescriptionLanguageParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.push_back( 
      ( "{" >> ruleset.r_parameterset >> "}" >> stringRule<Iterator,Skipper>().copy() ) 
      [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
    );
  }
};



template <typename Iterator, typename Skipper = qi::space_type >
struct ParameterDescriptionLanguageParser
: qi::grammar<Iterator, ParameterSetData(), Skipper>
{

public:
  
  ParameterDescriptionLanguageParserRuleset<Iterator,Skipper> rules;
    
  ParameterDescriptionLanguageParser()
  : ParameterDescriptionLanguageParser::base_type(rules.r_parameterset)
  {
    DoubleParameterParser::insertrule<Iterator, Skipper>(rules);
    IntParameterParser::insertrule<Iterator, Skipper>(rules);
    SubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    
    rules.init();

    on_error<fail>(rules.r_parameterset,
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
  
  BOOST_FOREACH(ParameterSetEntry& pe, result)
  {
    pe.second->writeCppHeader(cout, pe.first);
  }
  
  return 0;
}