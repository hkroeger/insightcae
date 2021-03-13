#ifndef INSIGHT_OPENFOAMBOUNDARYDICT_H
#define INSIGHT_OPENFOAMBOUNDARYDICT_H

#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamdict.h"

namespace insight {



class OpenFOAMBoundaryDict
        : public OFDictData::dictFile
{
  boost::filesystem::path caseDir_;
  std::string region_;
  std::string time_;

public:
  /**
   * @brief OpenFOAMBoundaryDict
   * read constructor
   * @param cm
   * @param caseDir
   * @param region
   * @param time
   */
  OpenFOAMBoundaryDict(const OpenFOAMCase& cm, const boost::filesystem::path& caseDir,
                       const std::string& region = std::string(),
                       const std::string& time = "constant" );

  void removeZeroSizedPatches();

  bool patchExists(const std::string& name) const;

  void renamePatch(
      const std::string& currentName,
      const std::string& newName,
      const std::string& newType = std::string() );

  void write() const;
};



} // namespace insight

#endif // INSIGHT_OPENFOAMBOUNDARYDICT_H
