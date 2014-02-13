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


#include "openfoamdict.h"

#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;

namespace insight
{


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
    out<<"FoamFile"<<endl
       <<"{"<<endl
       <<" version     "+lexical_cast<std::string>(d.dictVersion)+";"<<endl
       <<" format      ascii;"<<endl
       <<" class       "+d.className+";"<<endl
       <<" object      " << objname << ";"<<endl
       <<"}"<<endl;

    for (OFDictData::dict::const_iterator i=d.begin(); i!=d.end(); i++)
      out<< i->first << " " << i->second << ";\n";
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
	  std::cout << i->first << std::endl;
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

OFDictData::dictFile::dictFile()
: className("dictionary"),
  dictVersion(2),
  OFversion(-1)
{
}

OFDictData::list vector3(const arma::mat& v)
{
  OFDictData::list l;
  for (int i=0; i<v.n_rows; i++)
    l.push_back(v(i));
  return l;
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
  os << "{\n";
  for(dict::const_iterator i=d.begin(); i!=d.end(); i++)
  {
    os << i->first << SPACE;
    if (const dict *d = boost::get<dict>(&i->second))
    {
      //os << "\n{\n";
      os << *d;
      //os << "}\n";
    }
    else
    {
      os << i->second << ";\n";
    }
  }
  os << "}\n";
  return os;
}


std::ostream& operator<<(std::ostream& os, const list& l)
{
  os << "\n(\n";
  for (list::const_iterator i=l.begin(); i!=l.end(); i++)
  {
    os<<*i<<"\n";
  }
  os<<")\n";
  return os;
}

}

}