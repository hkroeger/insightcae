#include "propertylibraryselectionparameter.h"

#include "base/rapidxml.h"
#include "base/translations.h"

namespace insight {




defineType ( PropertyLibrarySelectionParameter );
addParameterFactories(PropertyLibrarySelectionParameter);




PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter(
        const std::string& description,
        bool isHidden,
        bool isExpert,
        bool isNecessary,
        int order )
    : StringParameter( description, isHidden, isExpert, isNecessary, order ),
      propertyLibrary_(nullptr)
{}




PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter(
    const PropertyLibraryBase& lib,
    const std::string& description,
    bool isHidden,
    bool isExpert,
    bool isNecessary,
    int order )
    : StringParameter( "", description, isHidden, isExpert, isNecessary, order ),
    propertyLibrary_ ( &lib )
{
    auto el = propertyLibrary_->entryList();
    if (el.size()>0)
    {
        setSelection(el.front());
    }
}




PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter(
        const std::string& value,
        const PropertyLibraryBase& lib,
        const std::string& description,
        bool isHidden,
        bool isExpert,
        bool isNecessary,
        int order )
    : StringParameter( value, description, isHidden, isExpert, isNecessary, order ),
      propertyLibrary_ ( &lib )
{
    if (contains(value))
    {
        setSelection(value);
    }
}




void PropertyLibrarySelectionParameter::initializeHierarchy()
{
    for (auto& e: instanceParameters_)
    {
        e.second->initializeHierarchy();
    }
}




bool PropertyLibrarySelectionParameter::isDifferent(const Parameter& p) const
{
    if (const auto *plsp = dynamic_cast<const PropertyLibrarySelectionParameter*>(&p))
    {
        if (StringParameter::isDifferent(p) || (plsp->propertyLibrary_!=propertyLibrary_))
            return true;

        if (value_.empty())
            return false;

        return instanceParameters().isDifferent(plsp->instanceParameters());
    }
    else return false;
}




const PropertyLibraryBase *PropertyLibrarySelectionParameter::propertyLibrary() const
{
    return propertyLibrary_;
}




void PropertyLibrarySelectionParameter::wireEntry(ParameterSet& ps)
{
    ps.setParent(this);
    ps.valueChanged.connect(childValueChanged);
    ps.childValueChanged.connect(childValueChanged);
}




ParameterSet& PropertyLibrarySelectionParameter::instanceParametersFor(const std::string& sel) const
{
    auto it = instanceParameters_.find(sel);
    if (it == instanceParameters_.end())
    {
        auto newParams = (propertyLibrary_ && !sel.empty()) ?
            propertyLibrary_->defaultParameters(sel) :
            ParameterSet::create();

        auto ins = instanceParameters_.insert({sel, std::move(newParams)});
        it = ins.first;

        const_cast<PropertyLibrarySelectionParameter*>(this)->wireEntry(*it->second);
        it->second->initializeHierarchy();
    }
    return *it->second;
}




ParameterSet& PropertyLibrarySelectionParameter::instanceParameters()
{
    return instanceParametersFor(selection());
}




const ParameterSet& PropertyLibrarySelectionParameter::instanceParameters() const
{
    return instanceParametersFor(selection());
}





std::vector<std::string> PropertyLibrarySelectionParameter::selectionKeys() const
{
    if (propertyLibrary_)
        return propertyLibrary_->entryList();
    else
        return {};
}




void PropertyLibrarySelectionParameter::setSelection ( const std::string& sel )
{
    insight::assertion(
        contains(sel),
        _("property library does not contain selection \"%s\"!\n"
        " Available values are: %s"),
        sel.c_str(), boost::join(selectionKeys(), " ").c_str() );

    // materialize (lazily build, if not yet visited) the target entry BEFORE
    // touching value_, so nAfter below reflects its real size
    ParameterSet& newParams = instanceParametersFor(sel);

    int nBefore=0, nAfter=0;
    if (!value_.empty())
    {
        nBefore = instanceParametersFor(value_).size();
    }
    nAfter = newParams.size();

    if (nBefore>0)
    {
        beforeChildRemoval(0, nBefore-1);
    }
    value_ = std::string();
    if (nBefore>0)
    {
        childRemovalDone(0, nBefore-1);
    }

    if (nAfter>0)
    {
        beforeChildInsertion(0, nAfter-1);
    }
    value_ = sel;
    if (nAfter>0)
    {
        childInsertionDone(0, nAfter-1);
    }

    triggerValueChanged();
}




const std::string& PropertyLibrarySelectionParameter::selection() const
{
    return value_;
}




std::string PropertyLibrarySelectionParameter::iconPathForKey(const std::string &key) const
{
    if (auto *pl = propertyLibrary())
    {
        return pl->icon(key);
    }
    return std::string();
}




rapidxml::xml_node<>*
PropertyLibrarySelectionParameter::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    const OutputProperties& outProps ) const
{
    auto* child = StringParameter::appendToNode(name, doc, node, outProps);

    instanceParameters().appendToNode(std::string(), doc, *child, outProps);

    return child;
}




const rapidxml::xml_node<>*
PropertyLibrarySelectionParameter::readFromNode(
    const std::string &name,
    const rapidxml::xml_node<> &node )
{
    // bypass StringParameter::readFromNode (which would set value_ directly,
    // silently, without going through setSelection's signal-sandwiched switch)
    auto *child = Parameter::readFromNode(name, node);

    if (child)
    {
        auto sel = getMandatoryAttribute(*child, "value");

        setSelection(sel);

        instanceParameters().readFromNode(std::string(), *child);

        triggerValueChanged();
    }

    return child;
}


PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter(
    const rapidxml::xml_node<> &node)
    : StringParameter(node),
      propertyLibrary_(nullptr)
{}



std::unique_ptr<hierarchicalData::Element> PropertyLibrarySelectionParameter::doCloneUninitialized() const
{
    std::unique_ptr<PropertyLibrarySelectionParameter> p;
    if (propertyLibrary_)
    {
        p=std::make_unique<PropertyLibrarySelectionParameter>(
            value_,
            *propertyLibrary_,
            description().simpleLatex(),
            isHidden(), isExpert(), isNecessary(),
            order()
            );
    }
    else
    {
        p=std::make_unique<PropertyLibrarySelectionParameter>(
            description().simpleLatex(),
            isHidden(), isExpert(), isNecessary(),
            order()
            );
    }

    // clone every visited entry (not just the active one), mirroring
    // SelectableSubsetParameter::doCloneUninitialized's "clone all alternatives"
    // loop. Uses map-index assignment (not insert()) since the constructor
    // above may already have lazily built a *default* entry for value_ -
    // this overwrites it with the real, cloned-from-this one.
    for (auto& e: instanceParameters_)
    {
        auto cloned = e.second->cloneAsUninitialized<ParameterSet>();
        p->wireEntry(*cloned);
        p->instanceParameters_[e.first] = std::move(cloned);
    }

    return p;
}




void PropertyLibrarySelectionParameter::assignFrom(const Element& e)
{
    auto& op=dynamic_cast<const PropertyLibrarySelectionParameter&>(e);

    propertyLibrary_ = op.propertyLibrary_;

    // reshape instanceParameters_ to match op.instanceParameters_: matching
    // entries are updated in place (identity preserved), missing ones cloned
    // in, stale ones dropped - mirroring ParameterSet::assignFrom/
    // SelectableSubsetParameter::assignFrom.
    std::set<std::string> unmatchedKeys;
    std::transform(
        instanceParameters_.begin(), instanceParameters_.end(),
        std::inserter(unmatchedKeys, unmatchedKeys.begin()),
        [](const InstanceParametersMap::value_type& v) { return v.first; } );

    for (auto& ov: op.instanceParameters_)
    {
        auto it = instanceParameters_.find(ov.first);
        if (it != instanceParameters_.end())
        {
            it->second->assignFrom(*ov.second);
            unmatchedKeys.erase(ov.first);
        }
        else
        {
            auto cloned = ov.second->cloneAsUninitialized<ParameterSet>();
            wireEntry(*cloned);
            cloned->initializeHierarchy();
            instanceParameters_.insert({ov.first, std::move(cloned)});
        }
    }

    for (auto& um: unmatchedKeys)
    {
        instanceParameters_.erase(um);
    }

    setSelection(op.selection());

    StringParameter::assignFrom(op);
}




void PropertyLibrarySelectionParameter::copyMatching(const Element& e)
{
    auto& op=dynamic_cast<const PropertyLibrarySelectionParameter&>(e);

    if (contains(op.selection()))
    {
        setSelection(op.selection());
        instanceParameters().copyMatching(op.instanceParameters());
    }

    Parameter::assignFrom(op);
}




void PropertyLibrarySelectionParameter::extend(const Element& op)
{
    auto& oplsp=dynamic_cast<const PropertyLibrarySelectionParameter&>(op);

    instanceParameters().extend(oplsp.instanceParameters());
}




bool PropertyLibrarySelectionParameter::isEqual(const insight::hierarchicalData::Element& op) const
{
    if (auto *oplsp = dynamic_cast<const PropertyLibrarySelectionParameter*>(&op))
    {
        if (selection()!=oplsp->selection())
            return false;

        return instanceParameters().isEqual(oplsp->instanceParameters());
    }
    else
        return false;
}




void PropertyLibrarySelectionParameter::resolveRelativePaths(
    const boost::filesystem::path &baseDirectory)
{
    // applies to every visited entry, not just the active one - matching
    // SelectableSubsetParameter::resolveRelativePaths
    for (auto& e: instanceParameters_)
    {
        e.second->resolveRelativePaths(baseDirectory);
    }
}




bool PropertyLibrarySelectionParameter::isPacked() const
{
    return instanceParameters().isPacked();
}




void PropertyLibrarySelectionParameter::pack()
{
    instanceParameters().pack();
}




void PropertyLibrarySelectionParameter::unpack(
    const boost::filesystem::path& basePath)
{
    instanceParameters().unpack(basePath);
}




void PropertyLibrarySelectionParameter::clearPackedData()
{
    instanceParameters().clearPackedData();
}




int PropertyLibrarySelectionParameter::nChildren() const
{
    if (value_.empty())
        return 0;
    return instanceParameters().size();
}




std::string PropertyLibrarySelectionParameter::childElementName(
    int i, bool redirectArrayElementsToDefault) const
{
    int nc = nChildren();
    if (i>=nc && i<(nc+(int)instanceParameters_.size()))
    {
        int j=i-nc;
        auto ii=instanceParameters_.begin();
        std::advance(ii, j);
        return "<"+ii->first+">";
    }
    return instanceParameters().childElementName(i, redirectArrayElementsToDefault);
}




std::string PropertyLibrarySelectionParameter::childElementName(
    const Element* childParam, bool redirectArrayElementsToDefault) const
{
    for (auto& e: instanceParameters_)
    {
        if (childParam==e.second.get())
            return std::string();
    }
    return instanceParameters().childElementName(childParam, redirectArrayElementsToDefault);
}




int PropertyLibrarySelectionParameter::childElementIndex(
    const std::string& name) const
{
    for (int k=0; k<nChildren()+(int)instanceParameters_.size(); ++k)
    {
        if (childElementName(k)==name) return k;
    }
    return -1;
}




hierarchicalData::Element&
PropertyLibrarySelectionParameter::childElementRef(int i)
{
    int nc = nChildren();
    if (i>=nc && i<(nc+(int)instanceParameters_.size()))
    {
        int j=i-nc;
        auto ii=instanceParameters_.begin();
        std::advance(ii, j);
        return *ii->second;
    }
    return instanceParameters().childElementRef(i);
}




const hierarchicalData::Element&
PropertyLibrarySelectionParameter::childElement(int i) const
{
    int nc = nChildren();
    if (i>=nc && i<(nc+(int)instanceParameters_.size()))
    {
        int j=i-nc;
        auto ii=instanceParameters_.begin();
        std::advance(ii, j);
        return *ii->second;
    }
    return instanceParameters().childElement(i);
}




} // namespace insight
