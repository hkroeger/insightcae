#include "meshingnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(MeshingNumerics);
addToOpenFOAMCaseElementFactoryTable(MeshingNumerics);

MeshingNumerics::MeshingNumerics(OpenFOAMCase& c, ParameterSetInput ip)
: decomposeParDict(c, ip.forward<Parameters>())
{
  // rename("MeshingNumerics");
}


void MeshingNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  //  std::cerr<<"Addd FVN "<<p_.decompWeights<<std::endl;

  // setup structure of dictionaries
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="none";
  controlDict["deltaT"]=1;
  controlDict["startFrom"]="latestTime";
  controlDict["startTime"]=0.0;
  controlDict["stopAt"]="endTime";
  controlDict["endTime"]=1000;
  controlDict["writeControl"]="timeStep";
  controlDict["writeInterval"]=1;
  controlDict["purgeWrite"]=0;
  controlDict["writeFormat"]="ascii";
  controlDict["writePrecision"]=8;
  if (OFversion()>=600)
  {
    controlDict["writeCompression"]="on";
  }
  else
  {
    controlDict["writeCompression"]="compressed";
  }
  controlDict["timeFormat"]="general";
  controlDict["timePrecision"]=6;
  controlDict["runTimeModifiable"]=true;
  controlDict.getList("libs");
  controlDict.subDict("functions");


  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  fvSolution.subDict("solvers");
  fvSolution.subDict("relaxationFactors");

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  fvSchemes.subDict("ddtSchemes");
  fvSchemes.subDict("gradSchemes");
  fvSchemes.subDict("divSchemes");
  fvSchemes.subDict("laplacianSchemes");
  fvSchemes.subDict("interpolationSchemes");
  fvSchemes.subDict("snGradSchemes");
  fvSchemes.subDict("fluxRequired");

  if (OFversion()>=300)
  {
    OFDictData::dict& wd = fvSchemes.subDict("wallDist");
    wd["method"]="meshWave";
    OFDictData::dict& wd2 = fvSchemes.subDict("patchDist");
    wd2["method"]="meshWave";
  }

  decomposeParDict::addIntoDictionaries(dictionaries);
}

} // namespace insight
