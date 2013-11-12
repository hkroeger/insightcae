
#include "parameter.h"
#include "base/latextools.h"

namespace insight 
{

Parameter::Parameter(const std::string& description)
: description_(description)
{
}

Parameter::~Parameter()
{
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

}