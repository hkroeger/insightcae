#include "propertylibraryselectionparameter.h"

namespace insight {

defineType ( PropertyLibrarySelectionParameter );

PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter(
        const std::string& description,
        bool isHidden,
        bool isExpert,
        bool isNecessary,
        int order )
    : StringParameter ( description, isHidden, isExpert, isNecessary, order ),
      propertyLibrary_(nullptr)
{
}

PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter(
    const PropertyLibraryBase& lib,
    const std::string& description,
    bool isHidden,
    bool isExpert,
    bool isNecessary,
    int order )
    : StringParameter ( "", description, isHidden, isExpert, isNecessary, order ),
    propertyLibrary_ ( &lib )
{
    auto el = propertyLibrary_->entryList();
    if (el.size()>0)
        setSelection(el.front());
}

PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter(
        const std::string& value,
        const PropertyLibraryBase& lib,
        const std::string& description,
        bool isHidden,
        bool isExpert,
        bool isNecessary,
        int order )
    : StringParameter ( value, description, isHidden, isExpert, isNecessary, order ),
      propertyLibrary_ ( &lib )
{
    setSelection(value);
}



bool PropertyLibrarySelectionParameter::isDifferent(const Parameter& p) const
{
    if (const auto *plsp = dynamic_cast<const PropertyLibrarySelectionParameter*>(&p))
    {
        return StringParameter::isDifferent(p) || (plsp->propertyLibrary_!=propertyLibrary_);
    }
    else return false;
}

std::vector<std::string> PropertyLibrarySelectionParameter::items() const
{
    if (propertyLibrary_)
        return propertyLibrary_->entryList();
    else
        return {};
}

bool PropertyLibrarySelectionParameter::contains(const std::string &value) const
{
    auto l = items();
    return ( std::find(l.begin(), l.end(), value) != l.end() );
}


void PropertyLibrarySelectionParameter::setSelection ( const std::string& sel )
{
    insight::assertion(
        contains(sel),
        "property library does not contain selection "+sel+"!\n"
        " Available values are: "+boost::join(items(), " ") );

    StringParameter::set(sel);
}

const std::string& PropertyLibrarySelectionParameter::selection() const
{
    return value_;
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


Parameter* PropertyLibrarySelectionParameter::clone() const
{
    if (propertyLibrary_)
    {
        return new PropertyLibrarySelectionParameter(
                value_,
                *propertyLibrary_,
                description_.simpleLatex(),
                isHidden_, isExpert_, isNecessary_,
                order_
                );
    }
    else
    {
        return new PropertyLibrarySelectionParameter(
                description_.simpleLatex(),
                isHidden_, isExpert_, isNecessary_,
                order_
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
