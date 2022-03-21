#include "toolkitversion.h"

#include <iostream>

#include "base/boost_include.h"

using namespace std;
using namespace boost;

namespace insight {


ToolkitVersion::ToolkitVersion(
    int majorVersion, int minorVersion, int patchVersion,
    const std::string& commit,
    const std::string& branch
    )
  : majorVersion_(majorVersion),
    minorVersion_(minorVersion),
    patchVersion_(patchVersion),
    commit_(commit),
    branch_(branch)
{

}

int ToolkitVersion::majorVersion() const
{
  return majorVersion_;
}

int ToolkitVersion::minorVersion() const
{
  return minorVersion_;
}


int ToolkitVersion::patchVersion() const
{
  return patchVersion_;
}


string ToolkitVersion::commit() const
{
  return commit_;
}


string ToolkitVersion::branch() const
{
  return branch_;
}


std::string ToolkitVersion::patchInfo() const
{
  return str(format("%d-%s (%s)") % patchVersion() % commit_ % branch_ );
}


std::string insight::ToolkitVersion::toString() const
{
  return str( format("%d.%d.%d-%s (%s)") % majorVersion() % minorVersion() % patchVersion() % commit() % branch() );
}


insight::ToolkitVersion::operator std::string() const
{
  return toString();
}



#define XSTR(a) STR(a)
#define STR(a) #a



const ToolkitVersion& ToolkitVersion::current()
{
    int pv=0;
    std::string commit="";

    std::string pvcommit = XSTR(INSIGHT_VERSION_PATCH);
    std::vector<std::string> res;
    boost::split(res, pvcommit, boost::is_any_of("-"));
    if (res.size()==2)
    {
        pv=boost::lexical_cast<int>(res[0]);
        commit=res[1];
    }
    static ToolkitVersion currentToolkitVersion(
                INSIGHT_VERSION_MAJOR, INSIGHT_VERSION_MINOR,
                pv, commit,
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
