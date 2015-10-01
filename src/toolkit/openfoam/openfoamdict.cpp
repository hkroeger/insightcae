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


#include "openfoamdict.h"

#define BOOST_SPIRIT_DEBUG

#include "boost/spirit/include/qi.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>

#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;

namespace insight
{


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
      using namespace qi;
      
        rquery =  *( rpair );
        rpair  =  ridentifier >> ( (rentry>>qi::lit(';')) | rsubdict | (rraw>>qi::lit(';'))) ;
        ridentifier  =  qi::lexeme[ alpha >> *(~char_("\"\\/;{}")-(eol|space)) >> !(~char_("\"\\/;{}")-(eol|space)) ];
	rstring = '"' >> *(~qi::char_('"')) >> '"';
	rraw = (~qi::char_("\"{}();") >> *(~qi::char_(';')) )|qi::string("");
	qi::real_parser<double, qi::strict_real_policies<double> > strict_double;
	rentry = ( strict_double | qi::int_ |  rdimensionedData | rlist | rstring | ridentifier );
	rdimensionedData = ridentifier >> qi::lit('[') >> qi::repeat(7)[qi::int_] >> qi::lit(']') >> rentry;
        rsubdict = qi::lit('{')
	      >> *(rpair) >> qi::lit('}');
        rlist = /*b*qi::char_("0-9") >> */ qi::lit('(')
	      >> *(rentry) >> qi::lit(')');

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
      using namespace qi;
      
        rquery =  rpair >> 
        *(qi::lit('0')|qi::lit('1')|qi::lit('2')|qi::lit('3')|qi::lit('4')|qi::lit('5')|qi::lit('6')|qi::lit('7')|qi::lit('8')|qi::lit('9')) 
	>> qi::lit('(') >> *(rpair) >> qi::lit(')');
        rpair  =  ridentifier >> ( (rentry>>qi::lit(';')) | rsubdict | (rraw>>qi::lit(';'))) ;
        ridentifier  =  qi::lexeme[ alpha >> *(~char_("\"\\/;{}")-(eol|space)) >> !(~char_("\"\\/;{}")-(eol|space)) ];
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
/*
	BOOST_SPIRIT_DEBUG_NODE(rquery);
	BOOST_SPIRIT_DEBUG_NODE(rpair);   
	BOOST_SPIRIT_DEBUG_NODE(ridentifier);
	BOOST_SPIRIT_DEBUG_NODE(rstring);
	BOOST_SPIRIT_DEBUG_NODE(rraw);
	BOOST_SPIRIT_DEBUG_NODE(rentry);
	BOOST_SPIRIT_DEBUG_NODE(rsubdict);
	BOOST_SPIRIT_DEBUG_NODE(rlist);
*/
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

void readOpenFOAMDict(std::istream& in, OFDictData::dict& d)
{
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    //in.close();
    
    cout<<parseOpenFOAMDict<OpenFOAMDictParser<std::string::iterator> >(contents.begin(), contents.end(), d)<<endl;
    
    // remove "FoamFile" entry, if present
    OFDictData::dict::iterator i=d.find("FoamFile");
    if (i!=d.end())
    {
      d.erase(i);
    }
    
    for(OFDictData::dict::const_iterator i=d.begin();
	i!=d.end(); i++)
	{
	  std::cout << i->first << std::endl;
	}
}

void writeOpenFOAMDict(std::ostream& out, const OFDictData::dictFile& d, const std::string& objname)
{
  out /*<< std::scientific*/ << std::setprecision(18);
    out<<"FoamFile"<<endl
       <<"{"<<endl
       <<" version     "+lexical_cast<std::string>(d.dictVersion)+";"<<endl
       <<" format      ascii;"<<endl
       <<" class       "+d.className+";"<<endl
       <<" object      " << objname << ";"<<endl
       <<"}"<<endl;

    for (OFDictData::dict::const_iterator i=d.begin(); i!=d.end(); i++)
    {
      out<< i->first << " " << i->second;
      const OFDictData::data* o=&(i->second);
      if (!boost::get<OFDictData::dict>(o)) out<<";";
      out << "\n";
    }
}

void readOpenFOAMBoundaryDict(std::istream& in, OFDictData::dict& d)
{
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    //in.close();

    //OFDictData::dict d; 
    cout << parseOpenFOAMDict<OpenFOAMBoundaryDictParser<std::string::iterator> >(contents.begin(), contents.end(), d) << endl;
    
    // remove "FoamFile" entry, if present
    OFDictData::dict::iterator i=d.find("FoamFile");
    if (i!=d.end())
    {
      d.erase(i);
    }
    
    for(OFDictData::dict::const_iterator i=d.begin();
	i!=d.end(); i++)
	{
	  std::cout << "\"" << i->first << "\"" << std::endl;
	}
   /* 
    OFDictData::list bl;
    for(OFDictData::dict::const_iterator i=d.begin();
	i!=d.end(); i++)
	{
	  //std::cout << i->first << std::endl;
	  bl.push_back( OFDictData::data(i->first) );
	  bl.push_back( i->second );
	}
    d2[""]=bl;
    */
}

void writeOpenFOAMBoundaryDict(std::ostream& out, const OFDictData::dictFile& d)
{
  typedef std::map<int, std::string> Ordering;
  Ordering ord;
  BOOST_FOREACH( const OFDictData::dictFile::value_type& i, d )
  {
    ord[d.subDict(i.first).getInt("startFace")] = i.first;
  }
  
  out /*<< std::scientific*/ << std::setprecision(18);
    out<<"FoamFile"<<endl
       <<"{"<<endl
       <<" version     "+lexical_cast<std::string>(d.dictVersion)+";"<<endl
       <<" format      ascii;"<<endl
       <<" class       "+d.className+";"<<endl
       <<" object      boundary;"<<endl
       <<"}"<<endl;

    out << d.size() << endl
	<< "(" << endl;
	
    //for (OFDictData::dict::const_iterator i=d.begin(); i!=d.end(); i++)
    BOOST_FOREACH( const Ordering::value_type& i, ord )
    {
      std::cout<<i.first<<" = "<<i.second<<std::endl;
      const OFDictData::dict& di = d.subDict(i.second);
      out<< i.second << " " << di << ";\n";
    }
    
    out << ")" << endl;
}

void writeOpenFOAMSequentialDict(std::ostream& out, const OFDictData::dictFile& d, const std::string& objname)
{
  out /*<< std::scientific*/ << std::setprecision(18);
    out<<"FoamFile"<<endl
       <<"{"<<endl
       <<" version     "+lexical_cast<std::string>(d.dictVersion)+";"<<endl
       <<" format      ascii;"<<endl
       <<" class       "+d.className+";"<<endl
       <<" object      " << objname << ";"<<endl
       <<"}"<<endl;

    for (OFDictData::dict::const_iterator i=d.begin(); i!=d.end(); i++)
    {
      out<< i->second;
//       const OFDictData::data* o=&(i->second);
      //if (!boost::get<OFDictData::dict>(o)) out<<";";
      out << "\n";
    }
}

bool patchExists(const OFDictData::dict& bd, const std::string& patchName)
{
  return (bd.find(patchName)!=bd.end());
}

namespace OFDictData
{

std::vector<int> dimension(int d0, int d1, int d2, int d3, int d4, int d5, int d6)
{
  std::vector<int> d(7);
  d[0]=d0;
  d[1]=d1;
  d[2]=d2;
  d[3]=d3;
  d[4]=d4;
  d[5]=d5;
  d[6]=d6;
  return d;
}

dimensionedData::dimensionedData()
: boost::fusion::tuple<
  std::string,
  std::vector<int>,
  data>()
{
}

dimensionedData::dimensionedData(const std::string& n, const std::vector<int>& dim, const data& d)
: boost::fusion::tuple<
  std::string,
  std::vector<int>,
  data>(n, dim, d)
{
}

list::list()
{}
    
    
void list::insertNoDuplicate(const OFDictData::data& d)
{
  std::vector< OFDictData::data >::const_iterator location;
  location = std::find( begin(), end(), d );
  if (location==end()) push_back(d);
}


dict& dict::addSubDictIfNonexistent(const std::string& key)
{
  dict::iterator i=find(key);
  if (i==end())
  {
    (*this)[key]=dict();
  } 
  return this->lookup<dict>(key);
}

list& dict::addListIfNonexistent(const std::string& key)
{
  dict::iterator i=find(key);
  if (i==end())
  {
    (*this)[key]=list();
  } 
  return this->lookup<list>(key);
}

void dict::write(std::ostream& os, int indentLevel) const
{
  std::string prec(indentLevel, ' ');
  std::string pren(indentLevel+1, ' ');
  
  os << prec << "{\n";
  for(dict::const_iterator i=begin(); i!=end(); i++)
  {
    os << pren << i->first << SPACE;
    if (const dict *d = boost::get<dict>(&i->second))
    {
      //os << *d;
      os<<"\n";
      d->write(os, indentLevel+1);
    }
    else
    {
      os << i->second << ";\n";
    }
  }
  os << prec << "}\n";
}

OFDictData::dictFile::dictFile()
: className("dictionary"),
  dictVersion(2),
  OFversion(-1),
  isSequential(false)
{
}

void OFDictData::dictFile::write(const boost::filesystem::path& dictPath) const
{
  if (!exists(dictPath.parent_path())) 
  {
    boost::filesystem::create_directories(dictPath.parent_path());
  }
  
  {
    std::ofstream f(dictPath.c_str());
    if (isSequential)
      writeOpenFOAMSequentialDict(f, *this, boost::filesystem::basename(dictPath));
    else
      writeOpenFOAMDict(f, *this, boost::filesystem::basename(dictPath));
  }
}

std::string to_OF(const arma::mat& v)
{
  std::string ret="";
  if (v.n_elem>1) ret="(";
  for (int i=0; i<v.n_elem; i++)
  {
    ret+=lexical_cast<string>(v(i));
    if (i != (v.n_elem-1) )
      ret+=" ";
  }
  if (v.n_elem>1) ret+=")";
  return ret;
}

OFDictData::list vector3(const arma::mat& v)
{
  return vector3(v(0), v(1), v(2));
}

OFDictData::data vectorSpace(const arma::mat& v)
{
  if (v.n_elem==1)
  {
    return as_scalar(v);
  }
  else
  {
    OFDictData::list l;
    for (int i=0; i<v.n_elem; i++)
      l.push_back(v(i));
    return l;
  }
}

OFDictData::list vector3(double x, double y, double z)
{
  OFDictData::list l;
  l.push_back(x);
  l.push_back(y);
  l.push_back(z);
  return l;
}

std::ostream& operator<<(std::ostream& os, const dimensionSet& d)
{
  os << "[ ";
  for (size_t i=0; i<7; i++) 
    os << d[i] << " ";
  os << "]";
  return os;
}

std::ostream& operator<<(std::ostream& os, const dimensionedData& d)
{
  os << boost::fusion::get<0>(d) << " " 
     << boost::fusion::get<1>(d) << " " 
     << boost::fusion::get<2>(d);
  return os;
}

std::ostream& operator<<(std::ostream& os, const dict& d)
{
  d.write(os);
//   os << "{\n";
//   for(dict::const_iterator i=d.begin(); i!=d.end(); i++)
//   {
//     os << i->first << SPACE;
//     if (const dict *d = boost::get<dict>(&i->second))
//     {
//       //os << "\n{\n";
//       os << *d;
//       //os << "}\n";
//     }
//     else
//     {
//       os << i->second << ";\n";
//     }
//   }
//   os << "}\n";
  return os;
}


std::ostream& operator<<(std::ostream& os, const list& l)
{
  std::string sep=" ";
  if (l.size()>9) sep="\n";
  
  os << "("+sep;
  for (list::const_iterator i=l.begin(); i!=l.end(); i++)
  {
    os<<*i<<sep;
  }
  os<<")";
  return os;
}

}

}