#ifndef INSIGHT_TOOLKITVERSION_H
#define INSIGHT_TOOLKITVERSION_H


#include <string>

namespace insight
{

class ToolkitVersion
{
  int majorVersion_, minorVersion_, patchVersion_;
  std::string commit_;
  std::string branch_;

public:
  ToolkitVersion(
      int majorVersion, int minorVersion, int patchVersion,
      const std::string& commit,
      const std::string& branch
      );

  int majorVersion() const;
  int minorVersion() const;
  int patchVersion() const;
  std::string commit() const;
  std::string branch() const;

  /**
   * @brief patchInfo
   * patchVersion-commit
   * @return
   */
  std::string patchInfo() const;

  std::string toString() const;
  operator std::string() const;

  static const ToolkitVersion& current();

  bool operator==(const ToolkitVersion& otv) const;
};



} // namespace insight

#endif // INSIGHT_TOOLKITVERSION_H
