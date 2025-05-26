#include "selectablesubsetparameter.h"

#include "base/cppextensions.h"
#include "base/exception.h"
#include "base/rapidxml.h"
#include "boost/range/adaptor/indexed.hpp"

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
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
  for ( auto &i: defaultValue )
  {
        addItem(
          i.first,
          std::dynamic_unique_ptr_cast<ParameterSet>(
              i.second->clone(false) ) );
  }
  setSelection(defaultSelection);
}



SelectableSubsetParameter::SelectableSubsetParameter(
    const key_type& defaultSelection,
    Entries&& defaultValue,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order)
    : Parameter(description, isHidden, isExpert, isNecessary, order)
{
  for ( auto &i: defaultValue )
  {
        addItem(i.first, std::move(i.second) );
  }
  setSelection(defaultSelection);
}




void SelectableSubsetParameter::initialize()
{
    for (auto &v: value_)
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



std::vector<std::string> SelectableSubsetParameter::selectionKeys() const
{
    std::vector<std::string> keys;
    for (auto& k: value_)
    {
        keys.push_back(k.first);
    }
    return keys;
}




void SelectableSubsetParameter::setSelection(const key_type &nk)
{
    insight::assertion(
        value_.count(nk),
        "selection \"%s\" is not valid ", nk.c_str() );

    int nBefore=0, nAfter=0;
    if (!selection_.empty())
    {
        nBefore=operator()().size();
    }
    nAfter=value_.at(nk)->size();

    if (nAfter>nBefore)
    {
        beforeChildInsertion(nBefore, nAfter-1);
    }
    else if (nAfter<nBefore)
    {
        beforeChildRemoval(nAfter, nBefore-1);
    }

    selection_=nk;

    if (nAfter>nBefore)
    {
        childInsertionDone(nBefore, nAfter-1);
    }
    else if (nAfter<nBefore)
    {
        childRemovalDone(nAfter, nBefore-1);
    }

    triggerValueChanged();
}




const SelectableSubsetParameter::key_type &SelectableSubsetParameter::selection() const
{
    return selection_;
}




// int SelectableSubsetParameter::selectionIndex() const
// {
//     return indexOfSelection(selection());
// }

// int SelectableSubsetParameter::indexOfSelection(const std::string &key) const
// {
//     for (auto s: boost::adaptors::index(value_))
//     {
//         if (s.value().first==key)
//             return s.index();
//     }
//     return -1;
// }

// void SelectableSubsetParameter::setSelectionFromIndex(int idx)
// {
//     if ((idx>=0) && (idx<value_.size()))
//     {
//         auto i=value_.begin();
//         std::advance(i, idx);
//         setSelection(i->first);
//     }
//     else
//         throw insight::Exception(
//             "index %d out of range 0 ... %d",
//             idx, value_.size() );
// }




SelectableSubsetParameter::EntryReferences
SelectableSubsetParameter::items() const
{
  EntryReferences result;
  for (auto &sp: value_)
  {
    result.insert({sp.first, sp.second});
  }
  return result;
}




SelectableSubsetParameter::Entries
SelectableSubsetParameter::copyItems() const
{
  Entries result;
  for (auto &sp: value_)
  {
    result.insert(
        {
         sp.first,
         std::dynamic_unique_ptr_cast<ParameterSet>(
                 sp.second->clone(false))
        });
  }
  return result;
}





void SelectableSubsetParameter::addItem(
    key_type key,
    std::unique_ptr<ParameterSet>&& ps )
{
    insight::assertion(
        key!=selection_,
        "inserted item must not be currently selected" );

    auto ins = value_.insert({key, std::move(ps)});

    ins.first->second->valueChanged
        .connect(childValueChanged);
    ins.first->second->childValueChanged
        .connect(childValueChanged);

    ins.first->second->setParent(this);

    // if (ins.first->first == selection_)
    // {
    //   triggerValueChanged();
    // }
}


void SelectableSubsetParameter::setParametersForSelection(
    const key_type& key, const ParameterSet& ps)
{
  value_.at(key)->merge(ps);
}


const ParameterSet&
SelectableSubsetParameter::getParametersForSelection(
    const key_type& key) const
{
  return *value_.at(key);
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




void SelectableSubsetParameter::unpack(
    const boost::filesystem::path& basePath)
{
  operator()().unpack(basePath);
}




void SelectableSubsetParameter::clearPackedData()
{
  operator()().clearPackedData();
}




rapidxml::xml_node<>*
SelectableSubsetParameter::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
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
    const rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  auto* child = findNode(node, name, type());
  if (child)
  {
    auto valuenode=child->first_attribute("value");
    insight::assertion(valuenode, "No value attribute present!");
    std::string sel=valuenode->value();

    setSelection(sel);

    if (value_.find(sel)==value_.end())
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




std::unique_ptr<Parameter>
SelectableSubsetParameter::clone (bool init) const
{
  auto np=
      std::make_unique<SelectableSubsetParameter>(
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );

  for (auto i=value_.begin(); i!=value_.end(); i++)
  {
    np->addItem(
          i->first,
          std::dynamic_unique_ptr_cast<ParameterSet>(
              i->second->clone(false))
          );
  }

  np->setSelection(selection_);

  if (init) np->initialize();

  return np;
}




void SelectableSubsetParameter::copyFrom(const Parameter& p)
{
  operator=(dynamic_cast<const SelectableSubsetParameter&>(p));
}




void SelectableSubsetParameter::operator=(const SelectableSubsetParameter& ossp)
{
  setSelection( ossp.selection_ );
  operator()() = ossp();

  Parameter::copyFrom(ossp);
}




void SelectableSubsetParameter::extend (
    const Parameter& other )
{
  if (auto *ossp=dynamic_cast<const SelectableSubsetParameter*>(&other))
  {
    operator()().extend( *ossp );
  }
}




void SelectableSubsetParameter::merge (
    const Parameter& other )
{
  if (auto *ossp=dynamic_cast<const SelectableSubsetParameter*>(&other))
  {
    setSelection( ossp->selection_ );
    operator()().merge( (*ossp)() );
  }
}




std::unique_ptr<Parameter>
SelectableSubsetParameter::intersection(
    const Parameter &other) const
{
  if (auto *ossp = dynamic_cast<const SelectableSubsetParameter*>(&other))
  {
    auto np = std::make_unique<SelectableSubsetParameter>(
        description().simpleLatex(),
          isHidden(), isExpert(), isNecessary(), order() );

    for (auto &ci: value_)
    {
      auto key=ci.first;
      if (ossp->value_.count(key))
      {
          // key in both SSPs
          auto isd = ci.second->intersection(*ossp->value_.at(key));
          std::unique_ptr<ParameterSet> issp(
              dynamic_cast<ParameterSet*>(
                  isd.release()));
          if (issp.get())
          {
              np->addItem(
                  key,
                  issp->cloneParameterSet() );
          }
      }
    }

    if (np->value_.count(selection_))
    {
      np->setSelection(selection_);
    }
    else if (np->value_.count(ossp->selection_))
    {
      np->setSelection(ossp->selection_);
    }
    else if (np->value_.size())
    {
      np->setSelection(np->value_.begin()->first);
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


int SelectableSubsetParameter::childParameterIndex(
    const std::string &name) const
{
    for (int k=0; k<nChildren()+value_.size(); ++k)
    {
        if (childParameterName(k)==name) return k;
    }
    return -1;
}


std::string
SelectableSubsetParameter::childParameterName(
    int i, bool ) const
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

std::string
SelectableSubsetParameter::childParameterName(
    const Parameter *childParam,
    bool redirectArrayElementsToDefault
    ) const
{
    for (auto &seld: value_)
    {
        if (childParam==seld.second.get())
            return std::string();
    }
    return
        operator()().childParameterName(
            childParam,
            redirectArrayElementsToDefault );
}


Parameter&
SelectableSubsetParameter::childParameterRef ( int i )
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


const Parameter&
SelectableSubsetParameter::childParameter( int i ) const
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
