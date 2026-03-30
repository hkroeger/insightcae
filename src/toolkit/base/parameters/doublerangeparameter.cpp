#include "doublerangeparameter.h"
#include "base/rapidxml.h"

namespace insight
{




defineType(DoubleRangeParameter);
addParameterFactories(DoubleRangeParameter);



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
  triggerValueChanged();
}


void DoubleRangeParameter::clear()
{
  values_.clear();
  triggerValueChanged();
}


std::string DoubleRangeParameter::latexRepresentation(
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
  return toStringList(values_, "; ");
}

std::string DoubleRangeParameter::plainTextRepresentation(int indent) const
{
  return std::string(indent, ' ') + toStringList(values_, "; ")  + '\n';
}


std::unique_ptr<DoubleParameter>
DoubleRangeParameter::toDoubleParameter(RangeList::const_iterator i) const
{
  return std::make_unique<DoubleParameter>(*i, "realized from range iterator");
}


rapidxml::xml_node<>* DoubleRangeParameter::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    const OutputProperties& outProps ) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, outProps);

    appendAttribute(
        doc, *child, "values",
        toStringList(values_, " ") );

    return child;
}

const rapidxml::xml_node<>*
DoubleRangeParameter::readFromNode
(
    const std::string& name,
    const rapidxml::xml_node<>& node
)
{
  using namespace rapidxml;
  auto* child = Parameter::readFromNode(name, node);
  if (child)
  {
      values_ = toNumberList<std::set<double> >(
          getMandatoryAttribute(*child, "values"), " ");

      triggerValueChanged();
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % plainTextRepresentation(0)
           )
        );
  }
  return child;
}



DoubleRangeParameter::DoubleRangeParameter(const rapidxml::xml_node<> &node)
    : Parameter(node)
{
    values_ = toNumberList<std::set<double> >(
        getMandatoryAttribute(node, "values"), " ");
}


std::unique_ptr<hierarchicalData::Element> DoubleRangeParameter::cloneUninitialized() const
{
  auto p = std::make_unique<DoubleRangeParameter>(
        values_,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order());
    return p;
}




void DoubleRangeParameter::assignFrom(const Element& rhs)
{
  auto &op =dynamic_cast<const DoubleRangeParameter&>(rhs);

  values_ = op.values_;

  Parameter::assignFrom(op);
}


bool DoubleRangeParameter::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const DoubleRangeParameter*>(&op))
    {
        if (values_.size()!=oa->values_.size())
            return false;

        auto i=values_.begin();
        auto j=oa->values_.begin();

        while (i!=values_.end())
        {
            if (*i!=*j)
                return false;

            ++i; ++j;
        }

        return true;
    }
    else
        return false;
}


int DoubleRangeParameter::nChildren() const
{
  return 0;
}



}
