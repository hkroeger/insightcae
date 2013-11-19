
#include "parameter.h"
#include "base/latextools.h"
#include "base/exception.h"

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

char DoubleName[] = "double";
char IntName[] = "int";
char BoolName[] = "bool";
char StringName[] = "string";
char PathName[] = "path";

template<> defineType(DoubleParameter);
template<> defineType(IntParameter);
template<> defineType(BoolParameter);
template<> defineType(StringParameter);
template<> defineType(PathParameter);

rapidxml::xml_node<> *Parameter::findNode(rapidxml::xml_node<>& father, const std::string& name)
{
  for (xml_node<> *child = father.first_node(type().c_str()); child; child = child->next_sibling(type().c_str()))
  {
    if (child->first_attribute("name")->value() == name)
    {
      return child;
    }
  }
  
  throw insight::Exception("No xml node found with type="+type()+" and name="+name);
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
      doc.allocate_string(boost::lexical_cast<std::string>(value_).c_str())
    ));
    return child;
}

void SelectionParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  value_=boost::lexical_cast<int>(child->first_attribute("value")->value());
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
  for(int i=0; i<defaultNum; i++)
  {
    insertValue( defaultFrom + (defaultTo-defaultFrom)*double(i)/double(defaultNum-1) );
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