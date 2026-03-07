#include "selectablesubsetparameter.h"

#include "base/cppextensions.h"
#include "base/exception.h"
#include "base/rapidxml.h"
#include "boost/range/adaptor/indexed.hpp"

#include <iterator>

namespace insight {




defineType(SelectableSubsetParameter);
addParameterFactories(SelectableSubsetParameter);






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
          i.second->cloneAs<ParameterSet>() );
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
    if (!nk.empty())
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

        // if (nAfter>nBefore)
        // {
        //     beforeChildInsertion(nBefore, nAfter-1);
        // }
        // else if (nAfter<nBefore)
        // {
        //     beforeChildRemoval(nAfter, nBefore-1);
        // }

        if (nBefore>0)
        {
            beforeChildRemoval(0, nBefore-1);
        }
        selection_=std::string();
        if (nBefore>0)
        {
            childRemovalDone(0, nBefore-1);
        }

        if (nAfter>0)
        {
            beforeChildInsertion(0, nAfter-1);
        }
        selection_=nk;
        if (nAfter>0)
        {
            childInsertionDone(0, nAfter-1);
        }

        // if (nAfter>nBefore)
        // {
        //     childInsertionDone(nBefore, nAfter-1);
        // }
        // else if (nAfter<nBefore)
        // {
        //     childRemovalDone(nAfter, nBefore-1);
        // }

        triggerValueChanged();
    }
}




const SelectableSubsetParameter::key_type &SelectableSubsetParameter::selection() const
{
    // insight::assertion(
    //     !selection_.empty(),
    //     "internal error: attempt to access during value change" );
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
         sp.second->cloneAs<ParameterSet>()
        });
  }
  return result;
}





void SelectableSubsetParameter::addItem(
    key_type key,
    std::unique_ptr<ParameterSet>&& ps )
{
    insight::assertion(
        selection_.empty() || (key!=selection()),
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

void SelectableSubsetParameter::removeItem(key_type key)
{
    if (selection_==key)
    {
        // to be removing selected item, switch to next possibility
        auto k=selectionKeys();
        auto i=std::find(k.begin(), k.end(), key);
        auto j=i;

        j++;
        if (j==k.end()) // already at the end; try previous
        {
            j=i;
            j--;
            if (j==k.end()) // already at the end; nothing left
            {
                throw insight::Exception(
                    "tried to remove last selection entry"
                    );
            }
        }

        setSelection(*j);
    }

    if (value_.count(key))
    {
        auto i=value_.find(key);
        value_.erase(i);
    }
}


void SelectableSubsetParameter::setParametersForSelection(
    const key_type& key, const ParameterSet& ps)
{
  value_.at(key)->assignFrom(ps);
}


const ParameterSet&
SelectableSubsetParameter::getParametersForSelection(
    const key_type& key) const
{
  return *value_.at(key);
}


std::string SelectableSubsetParameter::latexRepresentation(
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
  std::ostringstream os;
  os<<"selected as ``"<<SimpleLatex(selection()).toLaTeX()<<"''\\\\"<<std::endl;
  os<<operator()().latexRepresentation(name, documentHierarchyLevel, fsi);
  return os.str();
}




std::string SelectableSubsetParameter::plainTextRepresentation(int indent) const
{
  std::ostringstream os;
  os<<"selected as \""<<SimpleLatex(selection()).toPlainText()<<"\"";
  if (operator()().size()>0)
  {
      os<<": \n";
      os<<operator()().plainTextRepresentation(indent+1);
  }
  return os.str();
}



void SelectableSubsetParameter::resolveRelativePaths(
    const boost::filesystem::path &baseDirectory)
{
    for (auto& v: value_)
    {
        v.second->resolveRelativePaths(baseDirectory);
    }
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
    const OutputProperties& outProps) const
{
  insight::CurrentExceptionContext ex(insight::VerbosityLevel::Loops, "appending selectable subset "+name+" to node "+node.name());

  using namespace rapidxml;

  xml_node<>* child = Parameter::appendToNode(name, doc, node, outProps);

  appendAttribute(doc, *child, "value", selection());

  operator()().appendToNode(std::string(), doc, *child, outProps);

  return child;
}





const rapidxml::xml_node<>*
SelectableSubsetParameter::readFromNode
(
    const std::string& name,
    const rapidxml::xml_node<>& node)
{
  using namespace rapidxml;
  auto* child = Parameter::readFromNode(name, node);
  if (child)
  {
    auto sel=getMandatoryAttribute(*child, "value");

    setSelection(sel);

    if (value_.find(sel)==value_.end())
      throw insight::Exception(
            "Invalid selection key during read of selectableSubset %s",
            name.c_str());

    operator()().readFromNode(std::string(), *child);

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



SelectableSubsetParameter::SelectableSubsetParameter(
    const rapidxml::xml_node<> &node)
    : Parameter(node)
{
    auto sel=getMandatoryAttribute(node, "value");
    addItem(
        sel,
        std::make_unique<ParameterSet>(node) );
    setSelection(sel);
}



std::unique_ptr<hierarchicalData::Element>
SelectableSubsetParameter::clone () const
{
  auto np=
      std::make_unique<SelectableSubsetParameter>(
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );

  for (auto i=value_.begin(); i!=value_.end(); i++)
  {
    np->addItem(
          i->first,
          i->second->cloneAs<ParameterSet>()
          );
  }

  np->setSelection(selection());

  return np;
}




void SelectableSubsetParameter::assignFrom(const Element &p)
{
    auto& ossp =dynamic_cast<const SelectableSubsetParameter&>(p);


    std::set<std::string> unmatchedKeys;
    std::transform(
        value_.begin(), value_.end(),
        std::last_inserter(unmatchedKeys),
        [](const decltype(value_)::value_type& e) { return e.first; }
        );

    for (auto &ov: ossp.value_)
    {
        if (value_.count(ov.first))
        {
            value_.at(ov.first)->assignFrom(*ov.second);
            unmatchedKeys.erase(ov.first);
        }
        else
        {
            addItem(ov.first, ov.second->cloneAs<ParameterSet>());
        }
    }

    setSelection( ossp.selection() );

    for (auto& um: unmatchedKeys)
    {
        removeItem(um);
    }


    Parameter::assignFrom(ossp);
}



void SelectableSubsetParameter::copyMatching(
    const Element& p)
{
    auto& ossp =dynamic_cast<const SelectableSubsetParameter&>(p);

    setSelection( ossp.selection() );
    if (!ossp.selection().empty())
    {
        operator()().copyMatching(ossp());
    }

    Parameter::assignFrom(ossp);
}



void SelectableSubsetParameter::extend (
    const Element& other )
{
    auto &ossp=dynamic_cast<const SelectableSubsetParameter&>(other);

    operator()().extend( ossp );
}



bool SelectableSubsetParameter::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const SelectableSubsetParameter*>(&op))
    {
        if (selection()!=oa->selection())
            return false;

        if (!value_.at(selection())->isEqual(
                *oa->value_.at(oa->selection()) ) )
            return false;

        return true;
    }
    else
        return false;
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
                  issp->cloneAs<ParameterSet>() );
          }
      }
    }

    if (np->value_.count(selection()))
    {
      np->setSelection(selection());
    }
    else if (np->value_.count(ossp->selection()))
    {
      np->setSelection(ossp->selection());
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
    if (selection_.empty()) // intermediate state for GUI during selection switch
        return 0;
    else
        return operator()().size();
}


int SelectableSubsetParameter::childElementIndex(
    const std::string &name) const
{
    for (int k=0; k<nChildren()+value_.size(); ++k)
    {
        if (childElementName(k)==name) return k;
    }
    return -1;
}


std::string
SelectableSubsetParameter::childElementName(
    int i, bool ) const
{
    if (i>=nChildren() && i<(nChildren()+value_.size()))
    {
        int j=i-nChildren();
        auto ii=value_.begin();
        std::advance(ii, j);
        return "<"+ii->first+">";
    }
  return operator()().childElementName(i);
}

std::string
SelectableSubsetParameter::childElementName(
    const Element *childParam,
    bool redirectArrayElementsToDefault
    ) const
{
    for (auto &seld: value_)
    {
        if (childParam==seld.second.get())
            return std::string();
    }
    return
        operator()().childElementName(
            childParam,
            redirectArrayElementsToDefault );
}


hierarchicalData::Element&
SelectableSubsetParameter::childElementRef ( int i )
{
    if (i>=nChildren() && i<(nChildren()+value_.size()))
    {
        int j=i-nChildren();
        auto ii=value_.begin();
        std::advance(ii, j);
        return *ii->second;
    }
  return operator()().childElementRef(i);
}


const hierarchicalData::Element&
SelectableSubsetParameter::childElement( int i ) const
{
    if (i>=nChildren() && i<(nChildren()+value_.size()))
    {
        int j=i-nChildren();
        auto ii=value_.begin();
        std::advance(ii, j);
        return *ii->second;
    }
  return operator()().childElement(i);
}

} // namespace insight
