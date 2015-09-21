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

#include "parameter.h"
#include "base/latextools.h"
#include "base/exception.h"

#include "boost/archive/iterators/base64_from_binary.hpp"
#include "boost/archive/iterators/binary_from_base64.hpp"
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>

namespace boost 
{ 
namespace filesystem
{
  
template < >
// path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt)
path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end)
{ 
  for( ; begin != end ; ++begin )
	  *this /= *begin;
  return *this;
}
// Return path when appended to a_From will resolve to same as a_To
boost::filesystem::path make_relative( boost::filesystem::path a_From, boost::filesystem::path a_To )
{
  a_From = boost::filesystem::absolute( a_From ); a_To = boost::filesystem::absolute( a_To );
  boost::filesystem::path ret;
  boost::filesystem::path::const_iterator itrFrom( a_From.begin() ), itrTo( a_To.begin() );
  // Find common base
  for( boost::filesystem::path::const_iterator toEnd( a_To.end() ), fromEnd( a_From.end() ) ; itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo; ++itrFrom, ++itrTo );
  // Navigate backwards in directory to reach previously found base
  for( boost::filesystem::path::const_iterator fromEnd( a_From.end() ); itrFrom != fromEnd; ++itrFrom )
  {
	  if( (*itrFrom) != "." )
		  ret /= "..";
  }
  // Now navigate down the directory branch
  ret.append( itrTo, a_To.end() );
  return ret;
}

  
} 
}

//namespace boost { namespace filesystem { using filesystem3::make_relative; } }

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;

namespace insight 
{
  
const std::string base64_padding[] = {"", "==","="};

std::string base64_encode(const std::string& s) 
{
  namespace bai = boost::archive::iterators;

  std::stringstream os;

  // convert binary values to base64 characters
  typedef bai::base64_from_binary
  // retrieve 6 bit integers from a sequence of 8 bit bytes
  <bai::transform_width<const char *, 6, 8> > base64_enc; // compose all the above operations in to a new iterator

  std::copy(base64_enc(s.c_str()), base64_enc(s.c_str() + s.size()),
            std::ostream_iterator<char>(os));

  os << base64_padding[s.size() % 3];
  return os.str();
}

std::string base64_decode(const std::string& s) 
{
  namespace bai = boost::archive::iterators;

  std::stringstream os;

  typedef bai::transform_width<bai::binary_from_base64<const char *>, 8, 6> base64_dec;

  unsigned int size = s.size();

  // Remove the padding characters, cf. https://svn.boost.org/trac/boost/ticket/5629
  if (size && s[size - 1] == '=') {
    --size;
    if (size && s[size - 1] == '=') --size;
  }
  if (size == 0) return std::string();

  std::copy(base64_dec(s.data()), base64_dec(s.data() + size),
            std::ostream_iterator<char>(os));

  return os.str();
}

defineType(Parameter);
defineFactoryTable(Parameter, std::string);

Parameter::Parameter()
{
}

Parameter::Parameter(const std::string& description)
: description_(description)
{
}

Parameter::~Parameter()
{
}

rapidxml::xml_node<>* Parameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = doc.allocate_node(node_element, doc.allocate_string(this->type().c_str()));
    node.append_node(child);
    child->append_attribute(doc.allocate_attribute
    (
      "name", 
      doc.allocate_string(name.c_str()))
    );
    return child;
}


std::string valueToString(const arma::mat& value)
{
  std::string s;
  for (int i=0; i<value.n_elem; i++)
  {
    if (i>0) s+=" ";
    s+=boost::lexical_cast<string>(value(i));
  }
  return s;
}


void stringToValue(const std::string& s, arma::mat& v)
{
  std::vector<double> vals;
  std::istringstream iss(s);
  while (!iss.eof())
  {
    double c;
    iss >> c;
    if (iss.fail()) break;
    vals.push_back(c);
  }
  v=arma::mat(vals.data(), vals.size(), 1);
}
 
char DoubleName[] = "double";
char IntName[] = "int";
char BoolName[] = "bool";
char VectorName[] = "vector";
char StringName[] = "string";
char PathName[] = "pathbase";

defineType(PathParameter);
addToFactoryTable(Parameter, PathParameter, std::string);

PathParameter::PathParameter(const string& description)
: SimpleParameter<boost::filesystem::path, PathName>(description)
{
}

PathParameter::PathParameter(const path& value, const string& description)
: SimpleParameter<boost::filesystem::path, PathName>(value, description)
{
}

bool PathParameter::isPacked() const
{
  return false;
}

void PathParameter::pack()
{
}

void PathParameter::unpack()
{
}

Parameter* PathParameter::clone() const
{
    return new PathParameter(value_, description_);
}


  
template<> defineType(DoubleParameter);
template<> defineType(IntParameter);
template<> defineType(BoolParameter);
template<> defineType(VectorParameter);
template<> defineType(StringParameter);
typedef SimpleParameter<boost::filesystem::path, PathName> PathParameterBase;
template<> defineType(PathParameterBase);

addToFactoryTable(Parameter, DoubleParameter, std::string);
addToFactoryTable(Parameter, IntParameter, std::string);
addToFactoryTable(Parameter, BoolParameter, std::string);
addToFactoryTable(Parameter, VectorParameter, std::string);
addToFactoryTable(Parameter, StringParameter, std::string);
// addToFactoryTable(Parameter, PathParameter, std::string);


rapidxml::xml_node<> *Parameter::findNode(rapidxml::xml_node<>& father, const std::string& name)
{
  for (xml_node<> *child = father.first_node(type().c_str()); child; child = child->next_sibling(type().c_str()))
  {
    if (child->first_attribute("name")->value() == name)
    {
      return child;
    }
  }
 
  cout<<"Warning: No xml node found with type="+type()+" and name="+name<<", default value is used."<<endl;
  //throw insight::Exception("No xml node found with type="+type()+" and name="+name);
  return NULL;
}

template<> rapidxml::xml_node<>* SimpleParameter<boost::filesystem::path, PathName>::appendToNode
(
  const std::string& name, 
  rapidxml::xml_document<>& doc, 
  rapidxml::xml_node<>& node, 
  boost::filesystem::path inputfilepath
) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    std::string relpath="";
    if (!value_.empty())
    {
      relpath=make_relative(inputfilepath, value_).string();
      cout<<relpath<<endl;
    }
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      doc.allocate_string(relpath.c_str())
    ));
    return child;
  
}

template<> void SimpleParameter<boost::filesystem::path, PathName>::readFromNode
(
  const std::string& name, 
  rapidxml::xml_document<>& doc, 
  rapidxml::xml_node<>& node, 
  boost::filesystem::path inputfilepath
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
  {
    boost::filesystem::path abspath(child->first_attribute("value")->value());
    if (!abspath.empty())
    {
      if (abspath.is_relative())
      {
	abspath = boost::filesystem::absolute(inputfilepath / abspath);
      }
  #if BOOST_VERSION < 104800
  #warning Conversion into canonical paths disabled!
  #else
      if (boost::filesystem::exists(abspath)) // avoid throwing errors during read of parameterset
	  abspath=boost::filesystem::canonical(abspath);
  #endif
    }
    cout<<"path="<<abspath<<endl;
    value_=abspath;
  }
  std::cout<<"done."<<std::endl;
}

defineType(DirectoryParameter);
addToFactoryTable(Parameter, DirectoryParameter, std::string);

DirectoryParameter::DirectoryParameter(const std::string& description)
: PathParameter(".", description)
{}

DirectoryParameter::DirectoryParameter(const boost::filesystem::path& value, const std::string& description)
: PathParameter(value, description)
{}

std::string DirectoryParameter::latexRepresentation() const
{
    return std::string() 
      + "{\\ttfamily "
      + cleanSymbols( boost::lexical_cast<std::string>(boost::filesystem::absolute(value_)) )
      + "}";
}

Parameter* DirectoryParameter::clone() const
{
  return new DirectoryParameter(value_, description_);
}

rapidxml::xml_node<>* DirectoryParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      doc.allocate_string(value_.c_str())
    ));
    return child;
}

void DirectoryParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
    value_=boost::filesystem::path(child->first_attribute("value")->value());
}

defineType(SelectionParameter);
addToFactoryTable(Parameter, SelectionParameter, std::string);

SelectionParameter::SelectionParameter( const std::string& description)
: SimpleParameter< int , IntName>(-1, description)
{
}

SelectionParameter::SelectionParameter(const int& value, const SelectionParameter::ItemList& items, const std::string& description)
: SimpleParameter< int , IntName>(value, description),
  items_(items)
{
}

SelectionParameter::SelectionParameter(const std::string& key, const SelectionParameter::ItemList& items, const std::string& description)
: SimpleParameter< int , IntName>(0, description),
  items_(items)
{
  ItemList::const_iterator i=std::find(items_.begin(), items_.end(), key);
  if (i!=items_.end()) 
  {
    value_ = i - items_.begin();
  }
  else
    value_=0;
}

SelectionParameter::~SelectionParameter()
{
}

const SelectionParameter::ItemList& SelectionParameter::items() const
{ 
  return items_;
}

std::string SelectionParameter::latexRepresentation() const
{
  return cleanSymbols(items_[value_]);
}

Parameter* SelectionParameter::clone() const
{
  return new SelectionParameter(value_, items_, description_);
}

rapidxml::xml_node<>* SelectionParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      //doc.allocate_string( boost::lexical_cast<std::string>(value_).c_str() )
      doc.allocate_string( items_[value_].c_str() )
    ));
    return child;
}

void SelectionParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
  {
    //value_=boost::lexical_cast<int>(child->first_attribute("value")->value());
    string key=child->first_attribute("value")->value();
    ItemList::const_iterator i=std::find(items_.begin(), items_.end(), key);
    if (i!=items_.end()) 
    {
      value_ = i - items_.begin();
    }
    else
    {
      try
      {
	int v=boost::lexical_cast<int>(key);
	value_=v;
      }
      catch(...)
      {
	throw insight::Exception("Invalid selection value: "+key);
      }
    }
  }
}

defineType(DoubleRangeParameter);
addToFactoryTable(Parameter, DoubleRangeParameter, std::string);

DoubleRangeParameter::DoubleRangeParameter(const std::string& description)
: Parameter(description)
{
}

DoubleRangeParameter::DoubleRangeParameter(const RangeList& value, const std::string& description)
: Parameter(description),
  values_(value)
{
}


DoubleRangeParameter::DoubleRangeParameter(double defaultFrom, double defaultTo, int defaultNum, const std::string& description)
: Parameter(description)
{
  if (defaultNum==1)
    insertValue(defaultFrom);
  else
  {
    for(int i=0; i<defaultNum; i++)
    {
      insertValue( defaultFrom + (defaultTo-defaultFrom)*double(i)/double(defaultNum-1) );
    }
  }
}

DoubleRangeParameter::~DoubleRangeParameter()
{}

std::string DoubleRangeParameter::latexRepresentation() const
{
  std::ostringstream oss;
  oss << *values_.begin();
  for ( RangeList::const_iterator i=(++values_.begin()); i!=values_.end(); i++ )
  {
    oss<<"; "<<*i;
  }
  return oss.str();
}

DoubleParameter* DoubleRangeParameter::toDoubleParameter(RangeList::const_iterator i) const
{
  return new DoubleParameter(*i, "realized from range iterator");
}

Parameter* DoubleRangeParameter::clone() const
{
  return new DoubleRangeParameter(values_, description_);
}

rapidxml::xml_node<>* DoubleRangeParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    
    std::ostringstream oss;
    oss << *values_.begin();
    for ( RangeList::const_iterator i=(++values_.begin()); i!=values_.end(); i++ )
    {
      oss<<" "<<*i;
    }
    child->append_attribute(doc.allocate_attribute
    (
      "values", 
      doc.allocate_string(oss.str().c_str())
    ));
    return child;
}

void DoubleRangeParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
  {
    values_.clear();
    std::istringstream iss(child->first_attribute("values")->value());
    while (!iss.eof())
    {
      double v;
      iss >> v;
      //std::cout<<"read value="<<v<<std::endl;
      if (iss.fail()) break;
      values_.insert(v);
    }
  }
}

defineType(ArrayParameter);
addToFactoryTable(Parameter, ArrayParameter, std::string);

ArrayParameter::ArrayParameter(const std::string& description)
: Parameter(description)
{
}

ArrayParameter::ArrayParameter(const Parameter& defaultValue, int size, const std::string& description)
: Parameter(description),
  defaultValue_(defaultValue.clone())
{
  for (int i=0; i<size; i++) appendEmpty();
}
  
  
std::string ArrayParameter::latexRepresentation() const
{
  return std::string();
}
  
Parameter* ArrayParameter::clone () const
{
  ArrayParameter* np=new ArrayParameter(*defaultValue_, 0, description_);
  for (int i=0; i<size(); i++)
  {
    np->appendValue(value_[i]);
  }
  return np;
}

rapidxml::xml_node<>* ArrayParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
  cout<<"appending array "<<name<<endl;
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  defaultValue_->appendToNode("default", doc, *child, inputfilepath);
  for (int i=0; i<size(); i++)
  {
    value_[i].appendToNode(boost::lexical_cast<std::string>(i), doc, *child, inputfilepath);
  }
  return child;
}

void ArrayParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
  {
    value_.clear();
    for (xml_node<> *e = child->first_node(); e; e = e->next_sibling())
    {
      std::string name(e->first_attribute("name")->value());
      if (name=="default")
      {
// 	cout<<"reading default value"<<endl;
// 	defaultValue_.reset(Parameter::lookup(e->name(), ""));
	defaultValue_->readFromNode( name, doc, *child, inputfilepath );
      }
      else
      {
	int i=boost::lexical_cast<int>(name);
// 	cout<<"Reading element i="<<i<<endl;
	if (value_.size()<i+1) value_.resize(i+1, defaultValue_.get());
// 	cout<<"now at size="<<size()<<endl;
	value_[i].readFromNode( boost::lexical_cast<std::string>(i), doc, *child, inputfilepath );
      }
    }
  }
}
  
defineType(MatrixParameter);
addToFactoryTable(Parameter, MatrixParameter, std::string);

MatrixParameter::MatrixParameter(const std::string& description)
: Parameter(description)
{
}

MatrixParameter::MatrixParameter
(
  const arma::mat& defaultValue, 
  const string& description
)
: Parameter(description),
  value_(defaultValue)
{}


arma::mat& MatrixParameter::operator()()
{
  return value_;
}

const arma::mat& MatrixParameter::operator()() const
{
  return value_;
}

string MatrixParameter::latexRepresentation() const
{
  std::ostringstream oss;
  
  oss<<"\\begin{tabular}{l";
  for (int j=0;j<value_.n_cols; j++) oss<<'c'<<endl;
  oss<<"}\n";
  
  for (int i=0;i<value_.n_rows; i++)
  {
    oss<<i<<"&";
    for (int j=0;j<value_.n_cols; j++)
    {
      oss<<value_(i,j);
      if (j<value_.n_cols-1) oss<<"&";
    }
    oss<<"\\\\"<<endl;
  }
  oss<<"\\end{tabular}"<<endl;
  
  return oss.str();
}

xml_node< char >* MatrixParameter::appendToNode(const string& name, xml_document< char >& doc, xml_node< char >& node, path inputfilepath) const
{
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);

  std::ostringstream voss;
//   voss<<endl;
  value_.save(voss, arma::raw_ascii);
//   voss<<endl;
  
  // set stringified table values as node value
  child->value(doc.allocate_string(voss.str().c_str()));
    
  return child;
}

void MatrixParameter::readFromNode(const string& name, xml_document< char >& doc, xml_node< char >& node, path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
  {
    string value_str=child->value();
    std::istringstream iss(value_str);
    value_.load(iss, arma::raw_ascii);
  }
}

Parameter* MatrixParameter::clone() const
{
  return new MatrixParameter(value_, description_);
}


}