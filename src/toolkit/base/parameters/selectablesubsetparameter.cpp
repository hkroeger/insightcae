#include "selectablesubsetparameter.h"

namespace insight {




defineType(SelectableSubsetParameter);
addToFactoryTable(Parameter, SelectableSubsetParameter);




SelectableSubsetParameter::SelectableSubsetParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
}




SelectableSubsetParameter::SelectableSubsetParameter(const key_type& defaultSelection, const SubsetList& defaultValue, const std::string& description,
                                                     bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  selection_(defaultSelection)
{
  for ( const SelectableSubsetParameter::SingleSubset& i: defaultValue )
  {
    std::string key(boost::get<0>(i));
    value_.insert( ItemList::value_type(key, std::unique_ptr<ParameterSet>(boost::get<1>(i))) ); // take ownership of objects in given list!
  }
}


bool SelectableSubsetParameter::isDifferent(const Parameter& p) const
{
  if (const auto *ssp = dynamic_cast<const SelectableSubsetParameter*>(&p))
  {
    if (!(ssp->selection() == this->selection()))
      return true;

    return (*this)().ParameterSet::isDifferent( (*ssp)() );
  }
  else
    return true;
}

void SelectableSubsetParameter::addItem(key_type key, const ParameterSet& ps)
{
    value_.insert( ItemList::value_type(key, std::unique_ptr<ParameterSet>(ps.cloneParameterSet())) );
}




void SelectableSubsetParameter::setSelection(const key_type& key, const ParameterSet& ps)
{
    selection()=key;
    operator()().merge(ps);
}




std::string SelectableSubsetParameter::latexRepresentation() const
{
//  return "(Not implemented)";
  std::ostringstream os;
  os<<"selected as ``"<<SimpleLatex(selection_).toLaTeX()<<"''\\\\"<<std::endl;
  os<<operator()().latexRepresentation();
  return os.str();
}




std::string SelectableSubsetParameter::plainTextRepresentation(int indent) const
{
//  return "(Not implemented)";
  std::ostringstream os;
  os<<"selected as \""<<SimpleLatex(selection_).toPlainText()<<"\"";
  if (operator()().size()>0)
  {
      os<<": \n";
      os<<operator()().plainTextRepresentation(indent+1);
  }
  return os.str();
}




bool SelectableSubsetParameter::isPacked() const
{
  bool is_packed=false;
  auto& v = this->operator()(); // get active subset
  for (auto& p: v)
  {
    is_packed |= p.second->isPacked();
  }
  return is_packed;
}




void SelectableSubsetParameter::pack()
{
  auto& v = this->operator()(); // get active subset
  for (auto& p: v)
  {
    p.second->pack();
  }
}




void SelectableSubsetParameter::unpack(const boost::filesystem::path& basePath)
{
  auto& v = this->operator()(); // get active subset
  for (auto& p: v)
  {
    p.second->unpack(basePath);
  }
}




void SelectableSubsetParameter::clearPackedData()
{
  auto& v = this->operator()(); // get active subset
  for (auto& p: v)
  {
    p.second->clearPackedData();
  }
}




rapidxml::xml_node<>* SelectableSubsetParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
  insight::CurrentExceptionContext ex("appending selectable subset "+name+" to node "+node.name());

  using namespace rapidxml;

  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  child->append_attribute(doc.allocate_attribute
  (
    "value",
    doc.allocate_string(selection_.c_str())
  ));

  operator()().appendToNode(doc, *child, inputfilepath);

  return child;
}




void SelectableSubsetParameter::readFromNode
(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    auto valuenode=child->first_attribute("value");
    insight::assertion(valuenode, "No value attribute present!");
    selection_=valuenode->value();

    if (value_.find(selection_)==value_.end())
      throw insight::Exception("Invalid selection key during read of selectableSubset "+name);

    operator()().readFromNode(doc, *child, inputfilepath);
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




Parameter* SelectableSubsetParameter::clone () const
{
  SelectableSubsetParameter *np=new SelectableSubsetParameter(description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
  np->selection_=selection_;
  for (ItemList::const_iterator i=value_.begin(); i!=value_.end(); i++)
  {
    std::string key(i->first);
    np->value_.insert( ItemList::value_type(key, std::unique_ptr<ParameterSet>(i->second->cloneParameterSet())) );
  }
  return np;
}




void SelectableSubsetParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const SelectableSubsetParameter*>(&p))
  {
    Parameter::reset(p);
    selection_= op->selection_;
    for (const auto& v: op->value_)
    {
      std::string key(v.first);
      value_.insert( ItemList::value_type(key, std::unique_ptr<ParameterSet>(v.second->cloneParameterSet())) );
    }
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}




const ParameterSet& SelectableSubsetParameter::subset() const
{
  return (*this)();
}

} // namespace insight
