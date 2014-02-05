/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef INSIGHT_OPENFOAMDICT_H
#define INSIGHT_OPENFOAMDICT_H

#define BOOST_SPIRIT_DEBUG

#include <fstream>
#include <iostream>
#include <stack>
#include <functional>
#include <string>

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

#include "base/exception.h"
#include "base/linearalgebra.h"

namespace insight
{

 /*
class OpenFOAMDictEntry
{
public:
  virtual std::string toString() =0;
};


class PlainEntry
: public OpenFOAMDictEntry,
  public std::string
{
public:
  virtual std::string toString();
};


class ListEntry
: public OpenFOAMDictEntry,
  public boost::ptr_vector<OpenFOAMDictEntry>
{
public:
  virtual std::string toString();
};

class Dict
: public OpenFOAMDictEntry,
  public boost::ptr_map<std::string, OpenFOAMDictEntry>
{
public:
  virtual std::string toString();
};
*/
 

namespace OFDictData
{
  
  const char SPACE[]=" ";
  
typedef std::vector<int> dimensionSet;
dimensionSet dimension(int d0=0, int d1=0, int d2=0, int d3=0, int d4=0, int d5=0, int d6=0);
  
struct dict;
struct list;
struct dimensionedData;

typedef boost::variant<
    std::string,
    double,
    int,
    boost::recursive_wrapper<dimensionedData>,
    boost::recursive_wrapper<dict>,
    boost::recursive_wrapper<list>
    > data;
    
typedef std::pair<std::string, data> entry;

struct dimensionedData
: public boost::fusion::tuple<
  std::string,
  std::vector<int>,
  data>
{
    dimensionedData();
    //dimensionedData(const std::string&, const std::vector<int>&, const data&);
    dimensionedData(const std::string&, const std::vector<int>&, const data&);
};

struct list 
: public std::vector<data> 
{};

struct dict 
: public std::map<std::string, data> 
{
  
  template<class T>
  T& lookup(const std::string& key)
  {
    dict::iterator i=this->find(key);
    if (i==this->end())
      throw Exception("key "+key+" not found!");
    if (T *d = boost::get<T>(&i->second))
      return *d;
    else
      throw Exception("entry "+key+" is there but not of the requested type!");
  }
  
  template<class T>
  const T& lookup(const std::string& key) const
  {
    dict::const_iterator i=this->find(key);
    if (i==this->end())
      throw Exception("key "+key+" not found!");
    if (const T *d = boost::get<T>(&i->second))
      return *d;
    else
      throw Exception("entry "+key+" is there but not of the requested type!");
  }
  
  /**
   * add subdict, if no such key exists, return existing subdict otherwise
   * raise exception, if key exists and is not a subdict
   */
  dict& addSubDictIfNonexistent(const std::string& key);
  list& addListIfNonexistent(const std::string& key);
  
  inline dict& subDict(const std::string& key)
  {
    return this->lookup<dict>(key);
  }

  inline const dict& subDict(const std::string& key) const
  {
    return this->lookup<dict>(key);
  }

  inline list& getList(const std::string& key)
  {
    return this->lookup<list>(key);
  }

  inline double& getDouble(const std::string& key)
  {
    return this->lookup<double>(key);
  }

  inline const double& getDouble(const std::string& key) const
  {
    return this->lookup<double>(key);
  }

  inline int& getInt(const std::string& key)
  {
    return this->lookup<int>(key);
  }

  inline const int& getInt(const std::string& key) const
  {
    return this->lookup<int>(key);
  }

  inline std::string& getString(const std::string& key)
  {
    return this->lookup<std::string>(key);
  }

  inline const std::string& getString(const std::string& key) const
  {
    return this->lookup<std::string>(key);
  }

};

OFDictData::list vector3(const arma::mat& v);
OFDictData::list vector3(double x, double y, double z);

std::ostream& operator<<(std::ostream& os, const dimensionSet& d);
std::ostream& operator<<(std::ostream& os, const dimensionedData& d);
std::ostream& operator<<(std::ostream& os, const dict& d);
std::ostream& operator<<(std::ostream& os, const list& l);

}

/*
class OpenFOAMDict
: public Dict
{

public:
    OpenFOAMDict();
    OpenFOAMDict(const OpenFOAMDict& other);
    virtual ~OpenFOAMDict();
};
*/
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;

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

template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct OpenFOAMDictParser
  : qi::grammar<Iterator, OFDictData::dict(), Skipper>
{
    OpenFOAMDictParser()
      : OpenFOAMDictParser::base_type(rquery)
    {
        rquery =  *( rpair );
        rpair  =  ridentifier >> ( (rentry>>qi::lit(';')) | rsubdict | (rraw>>qi::lit(';'))) ;
        ridentifier  =  +(~qi::char_(" \"\\/;{}()\n"));
	rstring = '"' >> *(~qi::char_('"')) >> '"';
	rraw = (~qi::char_("\"{}();") >> *(~qi::char_(';')) )|qi::string("");
	rentry = (qi::int_ | qi::double_ | rdimensionedData | rlist | rstring | ridentifier );
	rdimensionedData = ridentifier >> qi::lit('[') >> qi::repeat(7)[qi::int_] >> qi::lit(']') >> rentry;
        rsubdict = qi::lit('{')
	      >> *(rpair) >> qi::lit('}');
        rlist = /*b*qi::char_("0-9") >> */ qi::lit('(')
	      >> *(rentry) >> qi::lit(')');

	//BOOST_SPIRIT_DEBUG_NODE(rquery);
	//BOOST_SPIRIT_DEBUG_NODE(rpair);
	      /*
	BOOST_SPIRIT_DEBUG_NODE(ridentifier);
	BOOST_SPIRIT_DEBUG_NODE(rstring);
	BOOST_SPIRIT_DEBUG_NODE(rraw);
	*/
	//BOOST_SPIRIT_DEBUG_NODE(rentry);
	//BOOST_SPIRIT_DEBUG_NODE(rsubdict);
	//BOOST_SPIRIT_DEBUG_NODE(rlist);

    }
    
    qi::rule<Iterator, OFDictData::dict(), Skipper> rquery;
    qi::rule<Iterator, OFDictData::entry(), Skipper> rpair;
    qi::rule<Iterator, std::string()> ridentifier;
    qi::rule<Iterator, std::string()> rstring;
    qi::rule<Iterator, std::string()> rraw;
    qi::rule<Iterator, OFDictData::data(), Skipper> rentry;
    qi::rule<Iterator, OFDictData::dimensionedData(), Skipper> rdimensionedData;
    qi::rule<Iterator, OFDictData::dict(), Skipper> rsubdict;
    qi::rule<Iterator, OFDictData::list(), Skipper> rlist;
    
};




template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct OpenFOAMBoundaryDictParser
  : qi::grammar<Iterator, OFDictData::dict(), Skipper>
{
    OpenFOAMBoundaryDictParser()
      : OpenFOAMBoundaryDictParser::base_type(rquery)
    {
        rquery =  rpair >> 
        *(qi::lit('0')|qi::lit('1')|qi::lit('2')|qi::lit('3')|qi::lit('4')|qi::lit('5')|qi::lit('6')|qi::lit('7')|qi::lit('8')|qi::lit('9')) 
	>> qi::lit('(') >> *(rpair) >> qi::lit(')');
        rpair  =  ridentifier >> ( (rentry>>qi::lit(';')) | rsubdict | (rraw>>qi::lit(';'))) ;
        ridentifier  =  +(~qi::char_(" \"\\/;{}()\n"));
	rstring = '"' >> *(~qi::char_('"')) >> '"';
	rraw = (~qi::char_("\"{}();") >> *(~qi::char_(';')) )|qi::string("");
	rentry = (qi::int_ | qi::double_ | rdimensionedData | rlist | rstring | ridentifier );
	rdimensionedData = ridentifier >> qi::lit('[') >> qi::repeat(7)[qi::int_] >> qi::lit(']') >> rentry;
        rsubdict = qi::lit('{')
	      >> *(rpair) >> qi::lit('}');
        rlist = /*qi::char_("0-9") >>*/
	      *(qi::lit('0')|qi::lit('1')|qi::lit('2')|qi::lit('3')|qi::lit('4')|qi::lit('5')|qi::lit('6')|qi::lit('7')|qi::lit('8')|qi::lit('9'))
	      >> qi::lit('(')
	      >> *(rentry) >> qi::lit(')');

	BOOST_SPIRIT_DEBUG_NODE(rquery);
	BOOST_SPIRIT_DEBUG_NODE(rpair);   
	BOOST_SPIRIT_DEBUG_NODE(ridentifier);
	BOOST_SPIRIT_DEBUG_NODE(rstring);
	BOOST_SPIRIT_DEBUG_NODE(rraw);
	BOOST_SPIRIT_DEBUG_NODE(rentry);
	BOOST_SPIRIT_DEBUG_NODE(rsubdict);
	BOOST_SPIRIT_DEBUG_NODE(rlist);

    }
    
    qi::rule<Iterator, OFDictData::dict(), Skipper> rquery;
    qi::rule<Iterator, OFDictData::entry(), Skipper> rpair;
    qi::rule<Iterator, std::string()> ridentifier;
    qi::rule<Iterator, std::string()> rstring;
    qi::rule<Iterator, std::string()> rraw;
    qi::rule<Iterator, OFDictData::data(), Skipper> rentry;
    qi::rule<Iterator, OFDictData::dimensionedData(), Skipper> rdimensionedData;
    qi::rule<Iterator, OFDictData::dict(), Skipper> rsubdict;
    qi::rule<Iterator, OFDictData::list(), Skipper> rlist;
    
};

template <typename Parser, typename Result, typename Iterator>
bool parseOpenFOAMDict(Iterator first, Iterator last, Result& d)
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


void readOpenFOAMDict(std::istream& in, OFDictData::dict& d);
void writeOpenFOAMDict(std::ostream& out, const OFDictData::dict& d, const std::string& objname);

void readOpenFOAMBoundaryDict(std::istream& in, OFDictData::dict& d);
void writeOpenFOAMBoundaryDict(std::ostream& out, const OFDictData::dict& d);

}

#endif // INSIGHT_OPENFOAMDICT_H
