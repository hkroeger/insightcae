#include "subsetparameter.h"

namespace insight
{


defineType(SubsetParameter);
addToFactoryTable(Parameter, SubsetParameter);


SubsetParameter::SubsetParameter()
{
}

SubsetParameter::SubsetParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
}

SubsetParameter::SubsetParameter(const ParameterSet& defaultValue, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  ParameterSet(defaultValue.entries())
{
}

bool SubsetParameter::isDifferent(const Parameter& p) const
{
  if (const auto *sp = dynamic_cast<const SubsetParameter*>(&p))
  {
    return (*this)().ParameterSet::isDifferent( (*sp)() );
  }
  else
    return true;
}

// void SubsetParameter::merge(const SubsetParameter& other)
// {
//   this->merge(other);
// }

std::string SubsetParameter::latexRepresentation() const
{
  return ParameterSet::latexRepresentation();
}

std::string SubsetParameter::plainTextRepresentation(int indent) const
{
  return "\n" + ParameterSet::plainTextRepresentation(indent+1);
}

bool SubsetParameter::isPacked() const
{
  bool is_packed=false;
  for(auto& p: *this)
  {
    is_packed |= p.second->isPacked();
  }
  return is_packed;
}

void SubsetParameter::pack()
{
  for(auto& p: *this)
  {
    p.second->pack();
  }
}

void SubsetParameter::unpack(const boost::filesystem::path& basePath)
{
  for(auto& p: *this)
  {
    p.second->unpack(basePath);
  }
}

void SubsetParameter::clearPackedData()
{
  for(auto& p: *this)
  {
    p.second->clearPackedData();
  }
}



rapidxml::xml_node<>* SubsetParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
  insight::CurrentExceptionContext ex("appending subset "+name+" to node "+node.name());

//   std::cout<<"appending subset "<<name<<std::endl;
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  ParameterSet::appendToNode(doc, *child, inputfilepath);
  return child;
}

void SubsetParameter::readFromNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    ParameterSet::readFromNode(doc, *child, inputfilepath);
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % plainTextRepresentation()
           )
        );
  }
}



Parameter* SubsetParameter::clone() const
{
  return new SubsetParameter(*this, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
}



const ParameterSet &SubsetParameter::subset() const
{
  return *this;
}

void SubsetParameter::merge(const SubParameterSet &other, bool allowInsertion)
{
  this->ParameterSet::merge(other.subset(), allowInsertion);
}

Parameter *SubsetParameter::intersection(const SubParameterSet &other) const
{
  return new SubsetParameter(
        this->ParameterSet::intersection(other.subset()),
      description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
}


}
