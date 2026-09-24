#include "propertylibrary.h"

namespace insight {

PropertyLibraryBase::PropertyLibraryBase(const std::string &libraryName)
    : libraryName_(libraryName)
{}



std::string PropertyLibraryBase::icon(const std::string &) const
{
    return std::string();
}


std::unique_ptr<ParameterSet> PropertyLibraryBase::defaultParameters(const std::string &label) const
{
    return ParameterSet::create();
}


} // namespace insight
