
#include "parameter.h"
#include "base/latextools.h"

using namespace rapidxml;

namespace insight 
{

Parameter::Parameter(const std::string& description)
: description_(description)
{
}

Parameter::~Parameter()
{
}

void Parameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
{
  xml_node<>* child = doc.allocate_node(node_element, name.c_str());
  node.append_node(child);
}


DirectoryParameter::DirectoryParameter(boost::filesystem::path defaultValue, const std::string& description)
: PathParameter(defaultValue, description)
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

SelectionParameter::SelectionParameter(int defaultValue, const SelectionParameter::ItemList& items, const std::string& description)
: SimpleParameter< int >(defaultValue, description),
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
  return std::string();
}

DoubleParameter* DoubleRangeParameter::toDoubleParameter(RangeList::const_iterator i) const
{
  return new DoubleParameter(*i, "realized from range iterator");
}

Parameter* DoubleRangeParameter::clone() const
{
  return new DoubleRangeParameter(values_, description_);
}

}