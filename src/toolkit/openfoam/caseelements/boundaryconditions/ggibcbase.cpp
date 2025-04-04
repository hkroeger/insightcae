#include "ggibcbase.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/openfoamtools.h"

namespace insight {




GGIBCBase::GGIBCBase(
    OpenFOAMCase& c, const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip )
: BoundaryCondition(
          c, patchName,
          boundaryDict,
          ip.forward<Parameters>() )
{}




void GGIBCBase::addIntoDictionaries ( OFdicts& dictionaries ) const
{
  BoundaryCondition::addIntoDictionaries(dictionaries);

  if (OFversion()<170)
  {
      auto& decomposeParDict = dictionaries.lookupDict("system/decomposeParDict");
      auto& gfz = decomposeParDict.getList("globalFaceZones");
      gfz.push_back( p().zone );
  }
  else if (OFversion()>=230)
  {
      auto& decomposeParDict = dictionaries.lookupDict("system/decomposeParDict");
      auto& constraints = decomposeParDict.subDict("constraints");
      auto& singleProcessorFaceSets = constraints.subDict("singleProcessorFaceSets");
      if (singleProcessorFaceSets.find("type")==singleProcessorFaceSets.end())
          singleProcessorFaceSets["type"]="singleProcessorFaceSets";
      auto& spfs = singleProcessorFaceSets.getList("singleProcessorFaceSets");
      spfs.push_back( OFDictData::list({p().zone, -1}) );
  }
}




void GGIBCBase::modifyMeshOnDisk(const OpenFOAMCase& cm, const boost::filesystem::path& location) const
{
    setSet(cm, location,
           {
               "faceSet "+p().zone+" new patchToFace "+patchName_
           });
    if (OFversion()<170)
    {
        cm.executeCommand(location, "setsToZones", { "-noFlipMap" } );
    }
}




} // namespace insight
