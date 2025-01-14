#include "propertylibraryselectionparameter.h"

#include "base/translations.h"

namespace insight {




defineType ( PropertyLibrarySelectionParameter );




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




void PropertyLibrarySelectionParameter::readFromNode(
    const std::string &name,
    rapidxml::xml_node<> &node,
    boost::filesystem::path p )
{
    StringParameter::readFromNode(name, node, p);
    insight::assertion(
        contains(value_),
        "invalid selection %s read from input data", value_.c_str() );
}




std::unique_ptr<Parameter> PropertyLibrarySelectionParameter::clone() const
{
    if (propertyLibrary_)
    {
        return std::make_unique<PropertyLibrarySelectionParameter>(
            value_,
            *propertyLibrary_,
            description().simpleLatex(),
            isHidden(), isExpert(), isNecessary(),
            order()
            );
    }
    else
    {
        return std::make_unique<PropertyLibrarySelectionParameter>(
            description().simpleLatex(),
            isHidden(), isExpert(), isNecessary(),
            order()
            );
    }
}




void PropertyLibrarySelectionParameter::copyFrom(const Parameter& p)
{
    operator=(dynamic_cast<const PropertyLibrarySelectionParameter&>(p));
}




void PropertyLibrarySelectionParameter::operator=(const PropertyLibrarySelectionParameter& op)
{
    propertyLibrary_ = op.propertyLibrary_;

    StringParameter::copyFrom(op);
}




int PropertyLibrarySelectionParameter::nChildren() const
{
    return 0;
}




} // namespace insight
