#include "doublerangeparameter.h"

namespace insight
{




defineType(DoubleRangeParameter);
addToFactoryTable(Parameter, DoubleRangeParameter);

DoubleRangeParameter::DoubleRangeParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
}

DoubleRangeParameter::DoubleRangeParameter(const RangeList& value, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  values_(value)
{
}


DoubleRangeParameter::DoubleRangeParameter(double defaultFrom, double defaultTo, int defaultNum, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order)
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

bool DoubleRangeParameter::isDifferent(const Parameter& p) const
{
  if (const auto *drp = dynamic_cast<const DoubleRangeParameter*>(&p))
  {
    if (drp->values().size() != values().size())
      return true;

    auto myi=values().begin(), oi=drp->values().begin();
    for (size_t i=0; i<values().size(); ++i)
    {
      if (*myi != *oi)
        return true;
      ++myi;
      ++oi;
    }

    return false;
  }
  else
    return true;
}

void DoubleRangeParameter::resetValues(const RangeList& nvs)
{
  values_=nvs;
  valueChanged();
}


void DoubleRangeParameter::clear()
{
  values_.clear();
  valueChanged();
}


std::string DoubleRangeParameter::latexRepresentation() const
{
  return toStringList(values_, "%g", "; ");

//  std::ostringstream oss;
//  oss << *values_.begin();
//  for ( RangeList::const_iterator i=(++values_.begin()); i!=values_.end(); i++ )
//  {
//    oss<<"; "<<*i;
//  }
//  return oss.str();
}

std::string DoubleRangeParameter::plainTextRepresentation(int indent) const
{
//  std::ostringstream oss;
//  oss << *values_.begin();
//  for ( RangeList::const_iterator i=(++values_.begin()); i!=values_.end(); i++ )
//  {
//    oss<<"; "<<*i;
//  }
  return std::string(indent, ' ') + toStringList(values_, "%g", "; ") /*oss.str()*/ + '\n';
}

DoubleParameter* DoubleRangeParameter::toDoubleParameter(RangeList::const_iterator i) const
{
  return new DoubleParameter(*i, "realized from range iterator");
}


rapidxml::xml_node<>* DoubleRangeParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);

//    std::ostringstream oss;
//    oss << *values_.begin();
//    for ( RangeList::const_iterator i=(++values_.begin()); i!=values_.end(); i++ )
//    {
//      oss<<" "<<*i;
//    }
    child->append_attribute(doc.allocate_attribute
    (
      "values",
      doc.allocate_string( /*oss.str()*/toStringList(values_, "%g", " ").c_str() )
    ));
    return child;
}

void DoubleRangeParameter::readFromNode
(
    const std::string& name,
    rapidxml::xml_node<>& node,
    boost::filesystem::path
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    values_.clear();
    std::istringstream iss(child->first_attribute("values")->value());
    while (!iss.eof())
    {
      double v;
      iss >> v;
      if (iss.fail()) break;
      values_.insert(v);
    }
    valueChanged();
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


Parameter* DoubleRangeParameter::clone() const
{
  return new DoubleRangeParameter(values_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
}



void DoubleRangeParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const DoubleRangeParameter*>(&p))
  {
    Parameter::reset(p);
    values_ = op->values_;
    valueChanged();
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}



}
