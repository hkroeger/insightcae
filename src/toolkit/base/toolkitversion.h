#ifndef INSIGHT_TOOLKITVERSION_H
#define INSIGHT_TOOLKITVERSION_H


#include <string>

namespace insight
{

class ToolkitVersion
{
  int majorVersion_, minorVersion_;
  std::string patchVersion_;
  std::string branch_;

public:
  ToolkitVersion(
      int majorVersion, int minorVersion,
      const std::string& patchVersion,
      const std::string& branch
      );

  int majorVersion() const;
  int minorVersion() const;
  std::string patchVersion() const;

  std::string patchInfo() const;

  std::string toString() const;
  operator std::string() const;

  static const ToolkitVersion& current();

  bool operator==(const ToolkitVersion& otv) const;
};



} // namespace insight

#endif // INSIGHT_TOOLKITVERSION_H
