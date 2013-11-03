
#include "parameter.h"

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

Parameter* SelectionParameter::clone() const
{
  return new SelectionParameter(value_, items_, description_);
}

}