#ifndef INSIGHT_TOOLKITVERSION_H
#define INSIGHT_TOOLKITVERSION_H


#include <string>

namespace insight
{

class ToolkitVersion
{

    ToolkitVersion();

public:
    int majorVersion() const;
    int minorVersion() const;

    std::string patchInfo() const;

    operator std::string() const;

    static ToolkitVersion current;
};



} // namespace insight

#endif // INSIGHT_TOOLKITVERSION_H
