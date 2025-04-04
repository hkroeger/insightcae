#include "selectionparameter.h"




namespace insight
{

bool SelectionParameterInterface::contains(const std::string &value) const
{
    auto l = selectionKeys();
    return ( std::find(l.begin(), l.end(), value) != l.end() );
}

int SelectionParameterInterface::indexOfSelection(const std::string& key) const
{
    auto keys=selectionKeys();
    for (auto s: boost::adaptors::index(keys))
    {
        if (s.value()==key)
            return s.index();
    }
    return -1;
}

int SelectionParameterInterface::selectionIndex() const
{
    return indexOfSelection(selection());
}

void SelectionParameterInterface::setSelectionFromIndex(int idx)
{
    auto keys=selectionKeys();
    if ((idx>=0) && (idx<keys.size()))
    {
        auto i=keys.begin();
        std::advance(i, idx);
        setSelection(*i);
    }
    else
        throw insight::Exception(
            "index %d out of range 0 ... %d",
            idx, keys.size() );
}

std::string
SelectionParameterInterface::iconPathForKey(
    const std::string &key ) const
{
    return std::string();
}


defineType(SelectionParameter);
addToFactoryTable(Parameter, SelectionParameter);

SelectionParameter::SelectionParameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order)
: IntParameter(-1, description, isHidden, isExpert, isNecessary, order)
{
}

SelectionParameter::SelectionParameter(
    const int& value,
    const SelectionParameter::ItemList& items,
    const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: IntParameter(value, description, isHidden, isExpert, isNecessary, order),
  items_(items)
{
}

SelectionParameter::SelectionParameter(
    const std::string& key,
    const SelectionParameter::ItemList& items,
    const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: IntParameter(0, description, isHidden, isExpert, isNecessary, order),
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

void SelectionParameter::resetItems(const ItemList &newItems)
{
  items_=newItems;
  value_=0;
  triggerValueChanged();
}


const SelectionParameter::ItemList& SelectionParameter::items() const
{
  return items_;
}

std::vector<std::string> SelectionParameter::selectionKeys() const
{
    return items_;
}

void SelectionParameter::setSelection ( const std::string& sel )
{
  value_=indexOfSelection ( sel );
  triggerValueChanged();
}

const std::string &SelectionParameter::selection() const
{
    return items_[value_];
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
    insight::CurrentExceptionContext ex(3, "appending selection "+name+" to node "+node.name());

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
}



std::unique_ptr<Parameter> SelectionParameter::clone(bool init) const
{
    auto p= std::make_unique<SelectionParameter>(
        value_, items_,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order()
        );
    if (init) p->initialize();
    return p;
}


void SelectionParameter::copyFrom(const Parameter& p)
{
  operator=(dynamic_cast<const SelectionParameter&>(p));

}

void SelectionParameter::operator=(const SelectionParameter& op)
{
  items_ = op.items_;

  IntParameter::copyFrom(op);
}

int SelectionParameter::nChildren() const
{
  return 0;
}




}
