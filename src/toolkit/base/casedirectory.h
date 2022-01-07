#ifndef INSIGHT_CASEDIRECTORY_H
#define INSIGHT_CASEDIRECTORY_H

#include "base/boost_include.h"

namespace insight {

class CaseDirectory;

typedef std::shared_ptr<CaseDirectory> CaseDirectoryPtr;

class CaseDirectory
    : public boost::filesystem::path
{
  bool keep_;
  bool isAutoCreated_;

public:
  explicit CaseDirectory(const boost::filesystem::path& path);
  explicit CaseDirectory(bool keep=true, const boost::filesystem::path& prefix="");
  ~CaseDirectory();

  void createDirectory();
  bool isAutoCreated() const;
  bool isExistingAndWillBeRemoved() const;
  bool isExistingAndNotEmpty() const;
  bool isPersistent() const;
  void makePersistent();

  static CaseDirectoryPtr makeTemporary(const boost::filesystem::path& prefix="");
};

} // namespace insight

#endif // INSIGHT_CASEDIRECTORY_H
