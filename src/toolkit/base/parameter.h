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


#ifndef INSIGHT_PARAMETER_H
#define INSIGHT_PARAMETER_H

#include "factory.h"
#include "base/latextools.h"
#include "base/linearalgebra.h"

#include <string>
#include <vector>
#include <string>
#include <typeinfo>
#include <set>

#include "base/boost_include.h"
#include <boost/noncopyable.hpp>
#include <boost/concept_check.hpp>
#include "boost/version.hpp"

#include "rapidxml/rapidxml.hpp"


namespace boost 
{ 
namespace filesystem
{
  
template < >
path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt);

boost::filesystem::path make_relative( boost::filesystem::path a_From, boost::filesystem::path a_To );
  
} 
}

//namespace boost { namespace filesystem { using filesystem3::make_relative; } }


namespace insight {
  
class Parameter
: boost::noncopyable
{
  
public:
  declareFactoryTable(Parameter, std::string);  
  
protected:
  std::string description_;
  
public:
  declareType("Parameter");
  
  Parameter();
  Parameter(const std::string& description);
  virtual ~Parameter();
  
  inline const std::string& description() const
  { return description_; }
  inline std::string& description() { return description_; }

  virtual std::string latexRepresentation() const =0;
  
  virtual rapidxml::xml_node<>* appendToNode
  (
    const std::string& name, 
    rapidxml::xml_document<>& doc, 
    rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath
  ) const;
  
  virtual void readFromNode
  (
    const std::string& name, 
    rapidxml::xml_document<>& doc, 
    rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath
  ) =0;

  rapidxml::xml_node<> *findNode(rapidxml::xml_node<>& father, const std::string& name);
  virtual Parameter* clone() const =0;
};


template<class V>
std::string valueToString(const V& value)
{
  return boost::lexical_cast<std::string>(value);
}

std::string valueToString(const arma::mat& value);

template<class V>
void stringToValue(const std::string& s, V& v)
{
  v=boost::lexical_cast<V>(s);
}

void stringToValue(const std::string& s, arma::mat& v);
  
template<class T, char const* N>
class SimpleParameter
: public Parameter
{
  
public:
  typedef T value_type;
  
protected:
  T value_;
  
public:
  declareType(N);

  SimpleParameter(const std::string& description)
  : Parameter(description)
  {}

  SimpleParameter(const T& value, const std::string& description)
  : Parameter(description),
    value_(value)
  {}
  
  virtual ~SimpleParameter()
  {}
  
  inline T& operator()() { return value_; }
  inline const T& operator()() const { return value_; }

  virtual std::string latexRepresentation() const
  {
    return cleanSymbols(valueToString(value_));
  }

  virtual Parameter* clone() const
  {
    return new SimpleParameter<T, N>(value_, description_);
  }

  virtual rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
  {
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      doc.allocate_string(valueToString(value_).c_str())
    ));
    return child;
  }

  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath)
  {
   std::cout<<"Reading simple "<<name<< std::endl;
  using namespace rapidxml;
    xml_node<>* child = findNode(node, name);
    if (child)
    {
      stringToValue(child->first_attribute("value")->value(), value_);
    }
  }
  
};


  
extern char DoubleName[];
extern char IntName[];
extern char BoolName[];
extern char VectorName[];
extern char StringName[];
extern char PathName[];

typedef SimpleParameter<double, DoubleName> DoubleParameter;
typedef SimpleParameter<int, IntName> IntParameter;
typedef SimpleParameter<bool, BoolName> BoolParameter;
typedef SimpleParameter<arma::mat, VectorName> VectorParameter;
typedef SimpleParameter<std::string, StringName> StringParameter;
typedef SimpleParameter<boost::filesystem::path, PathName> PathParameter;

#ifdef SWIG
%template(DoubleParameter) SimpleParameter<double, DoubleName>;
%template(IntParameter) SimpleParameter<int, IntName>;
%template(BoolParameter) SimpleParameter<bool, BoolName>;
%template(VectorParameter) SimpleParameter<arma::mat, VectorName>;
%template(StringParameter) SimpleParameter<std::string, StringName>;
%template(PathParameter) SimpleParameter<boost::filesystem::path, PathName>;
#endif

template<> rapidxml::xml_node<>* SimpleParameter<boost::filesystem::path, PathName>::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const;

template<> void SimpleParameter<boost::filesystem::path, PathName>::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
  boost::filesystem::path inputfilepath);

  
class DirectoryParameter
: public PathParameter
{
public:
  declareType("directory");
  
  DirectoryParameter(const std::string& description);
  DirectoryParameter(const boost::filesystem::path& value, const std::string& description);
  virtual std::string latexRepresentation() const;
  virtual Parameter* clone() const;
  virtual rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath);
};


class SelectionParameter
: public IntParameter
{
public:
  typedef std::vector<std::string> ItemList;
  
protected:
  ItemList items_;
  
public:
  declareType("selection");
  
  SelectionParameter(const std::string& description);
  SelectionParameter(const int& value, const ItemList& items, const std::string& description);
  SelectionParameter(const std::string& key, const ItemList& items, const std::string& description);
  virtual ~SelectionParameter();
  
  inline ItemList& items() { return items_; };
  virtual const ItemList& items() const;
  inline const std::string& selection() const { return items_[value_]; }

  virtual std::string latexRepresentation() const;
  
  virtual Parameter* clone() const;

  virtual rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath);
};

class DoubleRangeParameter
: public Parameter
{
public:
  typedef std::set<double> RangeList;
  
protected:
  RangeList values_;
  
public:
  typedef RangeList value_type;

  declareType("doubleRange");
  
  DoubleRangeParameter(const std::string& description);
  DoubleRangeParameter(const RangeList& value, const std::string& description);
  DoubleRangeParameter(double defaultFrom, double defaultTo, int defaultNum, const std::string& description);
  virtual ~DoubleRangeParameter();
  
  inline void insertValue(double v) { values_.insert(v); }
  inline RangeList::iterator operator()() { return values_.begin(); }
  inline RangeList::const_iterator operator()() const { return values_.begin(); }
  
  inline RangeList& values() { return values_; }
  inline const RangeList& values() const { return values_; }

  virtual std::string latexRepresentation() const;
  
  DoubleParameter* toDoubleParameter(RangeList::const_iterator i) const;
  
  virtual Parameter* clone() const;

  virtual rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath);
};


class ArrayParameter
: public Parameter
{
public:
  typedef boost::ptr_vector<Parameter> value_type;

protected:
  boost::shared_ptr<Parameter> defaultValue_;
  boost::ptr_vector<Parameter> value_;
  
public:
  declareType("array");
  
  ArrayParameter(const std::string& description);
  ArrayParameter(const Parameter& defaultValue, int size, const std::string& description);
  
  //inline void setParameterSet(const ParameterSet& paramset) { value_.reset(paramset.clone()); }
  inline void setDefaultValue(const Parameter& defP) { defaultValue_.reset(defP.clone()); }
  inline void eraseValue(int i) { value_.erase(value_.begin()+i); }
  inline void appendValue(const Parameter& np) { value_.push_back(np.clone()); }
  inline void appendEmpty() { value_.push_back(defaultValue_->clone()); }
  inline Parameter& operator[](int i) { return value_[i]; }
  inline const Parameter& operator[](int i) const { return value_[i]; }
  inline int size() const { return value_.size(); }
  
  virtual std::string latexRepresentation() const;
  
  virtual Parameter* clone () const;

  virtual rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath);
};


inline Parameter* new_clone(const Parameter& p)
{
  return p.clone();
}

}

#endif // INSIGHT_PARAMETER_H
