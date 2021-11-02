#include "toolkitversion.h"

#include <iostream>

#include "base/boost_include.h"

using namespace std;
using namespace boost;

namespace insight {


ToolkitVersion::ToolkitVersion(
    int majorVersion, int minorVersion,
    const std::string& patchVersion,
    const std::string& branch
    )
  : majorVersion_(majorVersion),
    minorVersion_(minorVersion),
    patchVersion_(patchVersion),
    branch_(branch)
{

}

int ToolkitVersion::majorVersion() const
{
//    return INSIGHT_VERSION_MAJOR;
  return majorVersion_;
}

int ToolkitVersion::minorVersion() const
{
//    return INSIGHT_VERSION_MINOR;
  return minorVersion_;
}


std::string ToolkitVersion::patchVersion() const
{
//    return std::string( XSTR(INSIGHT_VERSION_PATCH) );
  return patchVersion_;
}


std::string ToolkitVersion::patchInfo() const
{
//    return std::string( XSTR(INSIGHT_VERSION_PATCH) " (" XSTR(INSIGHT_BRANCH) ")" );
  return patchVersion() + " (" + branch_ + ")";
}


std::string insight::ToolkitVersion::toString() const
{
  return str( format("%d.%d.%s") % majorVersion() % minorVersion() % patchInfo() );
}


insight::ToolkitVersion::operator std::string() const
{
  return toString();
}



#define XSTR(a) STR(a)
#define STR(a) #a



const ToolkitVersion& ToolkitVersion::current()
{
  static ToolkitVersion currentToolkitVersion(
        INSIGHT_VERSION_MAJOR, INSIGHT_VERSION_MINOR,
        XSTR(INSIGHT_VERSION_PATCH),
        XSTR(INSIGHT_BRANCH) );
  return currentToolkitVersion;
}



bool ToolkitVersion::operator==(const ToolkitVersion &otv) const
{
  return
      ( majorVersion() == otv.majorVersion() )
      &&
      ( minorVersion() == otv.minorVersion() )
      &&
      ( patchVersion() == otv.patchVersion() )
      ;
}

} // namespace insight
