#include "selectionparameter.h"




namespace insight
{




defineType(SelectionParameter);
addToFactoryTable(Parameter, SelectionParameter);

SelectionParameter::SelectionParameter( const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: SimpleParameter< int , IntName>(-1, description, isHidden, isExpert, isNecessary, order)
{
}

SelectionParameter::SelectionParameter(const int& value, const SelectionParameter::ItemList& items, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: SimpleParameter< int , IntName>(value, description, isHidden, isExpert, isNecessary, order),
  items_(items)
{
}

SelectionParameter::SelectionParameter(const std::string& key, const SelectionParameter::ItemList& items, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: SimpleParameter< int , IntName>(0, description, isHidden, isExpert, isNecessary, order),
  items_(items)
{
  ItemList::const_iterator i=std::find(items_.begin(), items_.end(), key);
  if (i!=items_.end())
  {
    value_ = int( i - items_.begin() );
  }
  else
    value_ = 0;
}

SelectionParameter::~SelectionParameter()
{
}

bool SelectionParameter::isDifferent(const Parameter& p) const
{
  if (const auto *sp = dynamic_cast<const SelectionParameter*>(&p))
  {
    return selection()!=sp->selection();
  }
  else
    return true;
}


const SelectionParameter::ItemList& SelectionParameter::items() const
{
  return items_;
}

std::string SelectionParameter::latexRepresentation() const
{
  return SimpleLatex(items_[size_t(value_)]).toLaTeX();
}

std::string SelectionParameter::plainTextRepresentation(int) const
{
  return SimpleLatex(items_[size_t(value_)]).toPlainText();
}


rapidxml::xml_node<>* SelectionParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    child->append_attribute(doc.allocate_attribute
    (
      "value",
      //doc.allocate_string( boost::lexical_cast<std::string>(value_).c_str() )
      doc.allocate_string( items_[size_t(value_)].c_str() )
    ));
    return child;
}

void SelectionParameter::readFromNode
(
    const std::string& name,
    rapidxml::xml_document<>&,
    rapidxml::xml_node<>& node,
    boost::filesystem::path
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    //value_=boost::lexical_cast<int>(child->first_attribute("value")->value());
    std::string key=child->first_attribute("value")->value();
    ItemList::const_iterator i=std::find(items_.begin(), items_.end(), key);
    if (i != items_.end())
    {
      value_ = int( i - items_.begin() );
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
        throw insight::Exception("Invalid selection value ("+key+") in parameter "+name);
      }
    }
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
}



Parameter* SelectionParameter::clone() const
{
  return new SelectionParameter(value_, items_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
}


void SelectionParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const SelectionParameter*>(&p))
  {
    IntParameter::reset(p);
    items_ = op->items_;
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}




}
