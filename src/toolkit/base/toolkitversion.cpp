#include "toolkitversion.h"

#include <iostream>

#include "base/boost_include.h"

using namespace std;
using namespace boost;

namespace insight {

ToolkitVersion::ToolkitVersion()
{
}

int ToolkitVersion::majorVersion() const
{
    return INSIGHT_VERSION_MAJOR;
}

int ToolkitVersion::minorVersion() const
{
    return INSIGHT_VERSION_MINOR;
}


#define XSTR(a) STR(a)
#define STR(a) #a

std::string ToolkitVersion::patchInfo() const
{
    return std::string( XSTR(INSIGHT_VERSION_PATCH) " (" XSTR(INSIGHT_BRANCH) ")" );
}


insight::ToolkitVersion::operator std::string() const
{
    return str( format("%d.%d.%s") % majorVersion() % minorVersion() % patchInfo() );
}


ToolkitVersion ToolkitVersion::current;

} // namespace insight
