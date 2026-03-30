#include "propertylibraryselectionparameter.h"

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
        StringParameter::set(el.front());
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
    StringParameter::set(value);
}




bool PropertyLibrarySelectionParameter::isDifferent(const Parameter& p) const
{
    if (const auto *plsp = dynamic_cast<const PropertyLibrarySelectionParameter*>(&p))
    {
        return StringParameter::isDifferent(p) || (plsp->propertyLibrary_!=propertyLibrary_);
    }
    else return false;
}




const PropertyLibraryBase *PropertyLibrarySelectionParameter::propertyLibrary() const
{
    return propertyLibrary_;
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

    StringParameter::set(sel);
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




const rapidxml::xml_node<>*
PropertyLibrarySelectionParameter::readFromNode(
    const std::string &name,
    const rapidxml::xml_node<> &node )
{
    auto *child=StringParameter::readFromNode(name, node);

    insight::assertion(
        contains(value_),
        "invalid selection %s read from input data", value_.c_str() );

    return child;
}


PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter(
    const rapidxml::xml_node<> &node)
    : StringParameter(node)
{}



std::unique_ptr<hierarchicalData::Element> PropertyLibrarySelectionParameter::cloneUninitialized() const
{
    std::unique_ptr<Parameter> p;
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
    return p;
}






void PropertyLibrarySelectionParameter::assignFrom(const Element& e)
{
    auto& op=dynamic_cast<const PropertyLibrarySelectionParameter&>(e);

    propertyLibrary_ = op.propertyLibrary_;

    StringParameter::assignFrom(op);
}




int PropertyLibrarySelectionParameter::nChildren() const
{
    return 0;
}




} // namespace insight
