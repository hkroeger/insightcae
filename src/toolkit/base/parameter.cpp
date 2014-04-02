
#include "parameter.h"
#include "base/latextools.h"
#include "base/exception.h"

#include <iostream>

using namespace std;
using namespace boost;
using namespace rapidxml;

namespace insight 
{

defineType(Parameter);
defineFactoryTable(Parameter, std::string);

Parameter::Parameter(const std::string& description)
: description_(description)
{
}

Parameter::~Parameter()
{
}

rapidxml::xml_node<>* Parameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
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
  return 
  boost::lexical_cast<string>(value(0))
  +" "+
  boost::lexical_cast<string>(value(1))
  +" "+
  boost::lexical_cast<string>(value(2));
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
char PathName[] = "path";

template<> defineType(DoubleParameter);
template<> defineType(IntParameter);
template<> defineType(BoolParameter);
template<> defineType(VectorParameter);
template<> defineType(StringParameter);
template<> defineType(PathParameter);

addToFactoryTable(Parameter, DoubleParameter, std::string);
addToFactoryTable(Parameter, IntParameter, std::string);
addToFactoryTable(Parameter, BoolParameter, std::string);
addToFactoryTable(Parameter, VectorParameter, std::string);
addToFactoryTable(Parameter, StringParameter, std::string);
addToFactoryTable(Parameter, PathParameter, std::string);


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

defineType(DirectoryParameter);
addToFactoryTable(Parameter, DirectoryParameter, std::string);

DirectoryParameter::DirectoryParameter(const std::string& description)
: PathParameter(".", description)
{}

DirectoryParameter::DirectoryParameter(boost::filesystem::path value, const std::string& description)
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

rapidxml::xml_node<>* DirectoryParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node);
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      doc.allocate_string(value_.c_str())
    ));
    return child;
}

void DirectoryParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
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

SelectionParameter::SelectionParameter(int value, const SelectionParameter::ItemList& items, const std::string& description)
: SimpleParameter< int , IntName>(value, description),
  items_(items)
{
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

rapidxml::xml_node<>* SelectionParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node);
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      //doc.allocate_string( boost::lexical_cast<std::string>(value_).c_str() )
      doc.allocate_string( items_[value_].c_str() )
    ));
    return child;
}

void SelectionParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
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

rapidxml::xml_node<>* DoubleRangeParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node);
    
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

void DoubleRangeParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
  {
    std::istringstream iss(child->first_attribute("values")->value());
    while (!iss.eof())
    {
      double v;
      iss >> v;
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
    np->appendValue(value_[i]);
  return np;
}

rapidxml::xml_node<>* ArrayParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
{
  cout<<"appending array "<<name<<endl;
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node);
  defaultValue_->appendToNode("default", doc, *child);
  for (int i=0; i<size(); i++)
  {
    value_[i].appendToNode(boost::lexical_cast<std::string>(i), doc, *child);
  }
  return child;
}

void ArrayParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
{
  value_.clear();
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
  {
    for (xml_node<> *e = child->first_node(); e; e = e->next_sibling())
    {
      std::string name(e->first_attribute("name")->value());
      if (name=="default")
      {
	cout<<"reading default value"<<endl;
	defaultValue_.reset(Parameter::lookup(e->name(), ""));
	defaultValue_->readFromNode( name, doc, *child );
      }
      else
      {
	int i=boost::lexical_cast<int>(name);
	cout<<"Reading element i="<<i<<endl;
	if (value_.size()<i+1) value_.resize(i+1, defaultValue_.get());
	cout<<"now at size="<<size()<<endl;
	Parameter* curp = Parameter::lookup(e->name(), "");
	curp->readFromNode( boost::lexical_cast<std::string>(i), doc, *child );
	value_.replace(i, curp);
      }
    }
  }
}
  

}