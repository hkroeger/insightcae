#include "selectablesubsetparameter.h"

#include "base/cppextensions.h"
#include <iterator>

namespace insight {




defineType(SelectableSubsetParameter);
addToFactoryTable(Parameter, SelectableSubsetParameter);




SelectableSubsetParameter::SelectableSubsetParameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order)
{}




SelectableSubsetParameter::SelectableSubsetParameter(
    const key_type& defaultSelection,
    const EntryReferences& defaultValue,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  selection_(defaultSelection)
{
  for ( auto &i: defaultValue )
  {
        addItem(i.first, *i.second);
  }
}



SelectableSubsetParameter::SelectableSubsetParameter(
    const key_type& defaultSelection,
    const EntryCopies& defaultValue,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order)
    : Parameter(description, isHidden, isExpert, isNecessary, order),
    selection_(defaultSelection)
{
  for ( auto &i: defaultValue )
  {
        addItem(i.first, *i.second);
  }
}

void SelectableSubsetParameter::initialize()
{
    for (auto v: value_)
    {
        v.second->initialize();
    }
}




bool SelectableSubsetParameter::isDifferent(const Parameter& p) const
{
  if (const auto *ssp = dynamic_cast<const SelectableSubsetParameter*>(&p))
  {
    if (!(ssp->selection() == this->selection()))
      return true;

    return (*this)().isDifferent( (*ssp)() );
  }
  else
    return true;
}




void SelectableSubsetParameter::setSelection(const key_type &nk)
{
  selection_=nk;
  triggerValueChanged();
}




SelectableSubsetParameter::EntryReferences SelectableSubsetParameter::items() const
{
  EntryReferences result;
  for (auto sp: value_)
  {
    result.insert({sp.first, sp.second});
  }
  return result;
}




SelectableSubsetParameter::EntryCopies SelectableSubsetParameter::copyItems() const
{
  EntryCopies result;
  for (auto sp: value_)
  {
    result.insert(
        {
         sp.first,
         std::shared_ptr<SubsetParameter>(
             dynamic_cast<SubsetParameter*>(
                 sp.second->clone()) )
        });
  }
  return result;
}




void SelectableSubsetParameter::addItem(key_type key, const SubsetParameter& ps)
{
  auto ins = value_.insert(
      key,
      std::auto_ptr<SubsetParameter>(
          dynamic_cast<SubsetParameter*>(ps.clone()))
      );

  ins.first->second->valueChanged.connect(childValueChanged);
  ins.first->second->childValueChanged.connect(childValueChanged);

  ins.first->second->setParent(this);

  if (ins.first->first == selection_)
  {
    triggerValueChanged();
  }
}




void SelectableSubsetParameter::setParametersAndSelection(const key_type& key, const SubsetParameter& ps)
{
  selection_=key;
  setParametersForSelection(key, ps);
  triggerValueChanged();
}

void SelectableSubsetParameter::setParametersForSelection(const key_type& key, const SubsetParameter& ps)
{
  value_.at(key).merge(ps);
}


const SubsetParameter& SelectableSubsetParameter::getParametersForSelection(const key_type& key) const
{
  return value_.at(key);
}


std::string SelectableSubsetParameter::latexRepresentation() const
{
  std::ostringstream os;
  os<<"selected as ``"<<SimpleLatex(selection_).toLaTeX()<<"''\\\\"<<std::endl;
  os<<operator()().latexRepresentation();
  return os.str();
}




std::string SelectableSubsetParameter::plainTextRepresentation(int indent) const
{
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
  return operator()().isPacked();
}




void SelectableSubsetParameter::pack()
{
  operator()().pack();
}




void SelectableSubsetParameter::unpack(const boost::filesystem::path& basePath)
{
  operator()().unpack(basePath);
}




void SelectableSubsetParameter::clearPackedData()
{
  operator()().clearPackedData();
}




rapidxml::xml_node<>* SelectableSubsetParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
  insight::CurrentExceptionContext ex(3, "appending selectable subset "+name+" to node "+node.name());

  using namespace rapidxml;

  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  child->append_attribute(doc.allocate_attribute
  (
    "value",
    doc.allocate_string(selection_.c_str())
  ));

  operator()().appendToNode(std::string(), doc, *child, inputfilepath);

  return child;
}




void SelectableSubsetParameter::readFromNode
(
    const std::string& name,
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

    operator()().readFromNode(std::string(), *child, inputfilepath);

    triggerValueChanged();
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
  SelectableSubsetParameter *np=
      new SelectableSubsetParameter(
        description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);

  np->selection_=selection_;

  for (auto i=value_.begin(); i!=value_.end(); i++)
  {
    np->addItem(
          i->first, *i->second
          );
  }
  return np;
}




void SelectableSubsetParameter::copyFrom(const Parameter& p)
{
  operator=(dynamic_cast<const SelectableSubsetParameter&>(p));
}




void SelectableSubsetParameter::operator=(const SelectableSubsetParameter& ossp)
{
  selection_= ossp.selection_;
  operator()() = ossp();

  Parameter::copyFrom(ossp);
}




void SelectableSubsetParameter::extend ( const Parameter& other )
{
  if (auto *ossp=dynamic_cast<const SelectableSubsetParameter*>(&other))
  {
    operator()().extend( *ossp );
  }
}




void SelectableSubsetParameter::merge ( const Parameter& other )
{
  if (auto *ossp=dynamic_cast<const SelectableSubsetParameter*>(&other))
  {
    selection_= ossp->selection_;
    operator()().merge( (*ossp)() );
  }
}


std::unique_ptr<Parameter> SelectableSubsetParameter::intersection(const Parameter &other) const
{
  if (auto *ossp = dynamic_cast<const SelectableSubsetParameter*>(&other))
  {
    auto np = std::make_unique<SelectableSubsetParameter>(
        description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);

    for (auto ci: value_)
    {
      auto key=ci.first;
      if (ossp->value_.count(key))
      {
          // key in both SSPs
          auto isd = ci.second->intersection(ossp->value_.at(key));
          std::auto_ptr<SubsetParameter> issp(
              dynamic_cast<SubsetParameter*>(
                  isd.release()));
          if (issp.get())
          {
              np->addItem( key, *issp );
          }
      }
    }

    if (np->value_.count(selection_))
    {
      np->selection_=selection_;
    }
    else if (np->value_.count(ossp->selection_))
    {
      np->selection_=ossp->selection_;
    }
    else if (np->value_.size())
    {
      np->selection_=np->value_.begin()->first;
    }

    if (np->value_.size())
    {
      return np;
    }
  }

  return nullptr;
}




int SelectableSubsetParameter::nChildren() const
{
    return operator()().size();
}


int SelectableSubsetParameter::childParameterIndex(const std::string &name) const
{
    for (int k=0; k<nChildren()+value_.size(); ++k)
    {
        if (childParameterName(k)==name) return k;
    }
    return -1;
}


std::string SelectableSubsetParameter::childParameterName(int i) const
{
    if (i>=nChildren() && i<(nChildren()+value_.size()))
    {
        int j=i-nChildren();
        auto ii=value_.begin();
        std::advance(ii, j);
        return "<"+ii->first+">";
    }
  return operator()().childParameterName(i);
}


Parameter& SelectableSubsetParameter::childParameterRef ( int i )
{
    if (i>=nChildren() && i<(nChildren()+value_.size()))
    {
        int j=i-nChildren();
        auto ii=value_.begin();
        std::advance(ii, j);
        return *ii->second;
    }
  return operator()().childParameterRef(i);
}


const Parameter& SelectableSubsetParameter::childParameter( int i ) const
{
    if (i>=nChildren() && i<(nChildren()+value_.size()))
    {
        int j=i-nChildren();
        auto ii=value_.begin();
        std::advance(ii, j);
        return *ii->second;
    }
  return operator()().childParameter(i);
}

} // namespace insight
