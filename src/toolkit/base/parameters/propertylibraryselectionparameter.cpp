#include "propertylibraryselectionparameter.h"

namespace insight {

defineType ( PropertyLibrarySelectionParameter );

PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter (
        const std::string& description,
        bool isHidden,
        bool isExpert,
        bool isNecessary,
        int order )
    : StringParameter ( description, isHidden, isExpert, isNecessary, order ),
      propertyLibrary_(nullptr)
{
}

PropertyLibrarySelectionParameter::PropertyLibrarySelectionParameter (
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
    insight::assertion(
                contains(value),
                "property library does not contain selection "+value+"!\n"
                " Available values are: "+boost::join(items(), " ") );
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

void PropertyLibrarySelectionParameter::reset(const Parameter& p)
{
    if (const auto* op = dynamic_cast<const PropertyLibrarySelectionParameter*>(&p))
    {
      StringParameter::reset(p);
      propertyLibrary_ = op->propertyLibrary_;
    }
    else
      throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}

} // namespace insight
