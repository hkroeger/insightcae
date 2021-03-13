#include "openfoamboundarydict.h"

#include "boost/spirit/include/qi.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>

using namespace std;
using namespace boost;



namespace insight {




OpenFOAMBoundaryDict::OpenFOAMBoundaryDict(
    const OpenFOAMCase &cm,
    const boost::filesystem::path &caseDir,
    const std::string &region,
    const std::string &time )
  : caseDir_(caseDir), region_(region), time_(time)
{
  cm.parseBoundaryDict(caseDir_, *this, region_, time_);
}

void OpenFOAMBoundaryDict::removeZeroSizedPatches()
{
  bool erased;

  do
  {
    erased=false;
    for (auto i=begin(); i!=end(); ++i)
    {
      auto sd=subDict(i->first);
      if (sd.getInt("nFaces")<=0)
      {
        this->erase(i);
        erased=true;
        break;
      }
    }
  } while (erased);

}

bool OpenFOAMBoundaryDict::patchExists(const std::string &name) const
{
  return insight::patchExists(*this, name);
}

void OpenFOAMBoundaryDict::renamePatch(
    const std::string &currentName,
    const std::string &newName,
    const std::string &newType )
{
  auto i = find(currentName);

  if (i==end())
    throw insight::Exception("Boundary definition did not contain a patch named "+currentName);

  auto value = i->second;
  erase(i);
  if (!newType.empty())
  {
    OFDictData::dict& bi = boost::get<OFDictData::dict&>(value);
    bi["type"]=newType;
    auto i = bi.find("inGroups");
    if (i!=bi.end())
      bi.erase(i);
  }
  operator[](newName)=value;
}

void OpenFOAMBoundaryDict::write() const
{
  std::ofstream bf( (caseDir_/time_/"polyMesh"/"boundary").string() );
  insight::writeOpenFOAMBoundaryDict(bf, *this, false);
}

} // namespace insight
