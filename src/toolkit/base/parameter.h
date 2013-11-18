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


#ifndef INSIGHT_PARAMETER_H
#define INSIGHT_PARAMETER_H

#include <string>
#include <vector>
#include <string>
#include <typeinfo>
#include <set>

#include "boost/filesystem.hpp"
#include "boost/variant.hpp"
#include <boost/noncopyable.hpp>
#include <boost/concept_check.hpp>
#include <boost/lexical_cast.hpp>

#include "rapidxml/rapidxml.hpp"

namespace insight {
  
class Parameter
: boost::noncopyable
{
protected:
  std::string description_;
  
public:
  Parameter(const std::string& description);
  virtual ~Parameter();
  
  inline const std::string& description() const
  { return description_; }
  
  virtual std::string latexRepresentation() const =0;
  
  virtual void appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const =0;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) =0;

  rapidxml::xml_node<> *findNode(rapidxml::xml_node<>& father, const std::string& type, const std::string& name);
  virtual Parameter* clone() const =0;
};


template<class T, char const* N>
class SimpleParameter
: public Parameter
{
  
protected:
  T value_;
  
public:
  SimpleParameter(T defaultValue, const std::string& description)
  : Parameter(description),
    value_(defaultValue)
  {}
  
  virtual ~SimpleParameter()
  {}
  
  inline T& operator()() { return value_; }
  inline const T& operator()() const { return value_; }

  virtual std::string latexRepresentation() const
  {
    return boost::lexical_cast<std::string>(value_);
  }

  virtual Parameter* clone() const
  {
    return new SimpleParameter<T, N>(value_, description_);
  }

  virtual void appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
  {
    using namespace rapidxml;
    xml_node<>* child = doc.allocate_node(node_element, N);
    node.append_node(child);
    child->append_attribute(doc.allocate_attribute
    (
      "name", 
      doc.allocate_string(name.c_str()))
    );
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      doc.allocate_string(boost::lexical_cast<std::string>(value_).c_str())
    ));
  }

  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
  {
    using namespace rapidxml;
    xml_node<>* child = findNode(node, N, name);
    value_=boost::lexical_cast<T>(child->first_attribute("value")->value());
  }
  
};

extern char DoubleName[];
extern char IntName[];
extern char BoolName[];
extern char StringName[];
extern char PathName[];

typedef SimpleParameter<double, DoubleName> DoubleParameter;
typedef SimpleParameter<int, IntName> IntParameter;
typedef SimpleParameter<bool, BoolName> BoolParameter;
typedef SimpleParameter<std::string, StringName> StringParameter;
typedef SimpleParameter<boost::filesystem::path, PathName> PathParameter;

class DirectoryParameter
: public PathParameter
{
public:
  DirectoryParameter(boost::filesystem::path defaultValue, const std::string& description);
  virtual std::string latexRepresentation() const;
  virtual Parameter* clone() const;
  virtual void appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node);
};


class SelectionParameter
: public IntParameter
{
public:
  typedef std::vector<std::string> ItemList;
  
protected:
  ItemList items_;
  
public:
  SelectionParameter(int defaultValue, const ItemList& items, const std::string& description);
  virtual ~SelectionParameter();
  
  virtual const ItemList& items() const;

  virtual std::string latexRepresentation() const;
  
  virtual Parameter* clone() const;

  virtual void appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node);
};

class DoubleRangeParameter
: public Parameter
{
public:
  typedef std::set<double> RangeList;
  
protected:
  RangeList values_;
  
public:
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

  virtual void appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node);
};

}

namespace boost
{
  
inline insight::Parameter* new_clone(const insight::Parameter& p)
{
  return p.clone();
}

}

#endif // INSIGHT_PARAMETER_H
