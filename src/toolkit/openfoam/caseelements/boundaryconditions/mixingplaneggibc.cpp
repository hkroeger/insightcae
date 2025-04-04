#include "mixingplaneggibc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {




defineType(MixingPlaneGGIBC);
addToFactoryTable(BoundaryCondition, MixingPlaneGGIBC);
addToStaticFunctionTable(BoundaryCondition, MixingPlaneGGIBC, defaultParameters);




MixingPlaneGGIBC::MixingPlaneGGIBC
(
  OpenFOAMCase& c,
  const std::string& patchName,
  const OFDictData::dict& boundaryDict,
  ParameterSetInput ip
) : GGIBCBase(
          c, patchName,
          boundaryDict,
          ip.forward<Parameters>() )
{
  if ((OFversion()<160) || (OFversion()>=170))
  {
    throw insight::Exception("MixingPlane interface is not available in the selected OF version!");
  }
}




void MixingPlaneGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  bndDict["type"]="mixingPlane";
  bndDict["shadowPatch"]= p().shadowPatch;
  bndDict["separationOffset"]=OFDictData::vector3(p().separationOffset);
  bndDict["bridgeOverlap"]=p().bridgeOverlap;
  bndDict["zone"]=p().zone;

  OFDictData::dict csDict;
  csDict["type"]="cylindrical";
  csDict["name"]="mixingCS";
  csDict["origin"]=OFDictData::vector3(p().rotationCentre);
  arma::mat orthodir = arma::cross(p().rotationAxis, vec3(1,0,0));
  if (arma::norm(orthodir,2)<1e-6)
    orthodir=arma::cross(p().rotationAxis, vec3(0,1,0));
  csDict["e1"]=OFDictData::vector3(orthodir); //radial
  csDict["e3"]=OFDictData::vector3(p().rotationAxis); // rot axis

  if (OFversion()>=164)
    csDict["degrees"]=false;
  else
    csDict["inDegrees"]=false;

  bndDict["coordinateSystem"]=csDict;

  OFDictData::dict rpDict;
  rpDict["sweepAxis"]="Theta";
  if (p().stackAxisOrientation==Parameters::stackAxisOrientation_type::radial)
    rpDict["stackAxis"]="R"; // axial interface
  else if (p().stackAxisOrientation==Parameters::stackAxisOrientation_type::axial)
    rpDict["stackAxis"]="Z"; // axial interface
  rpDict["discretisation"]="bothPatches";
  bndDict["ribbonPatch"]=rpDict;

}




void MixingPlaneGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);

  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);

    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      BC["type"]=OFDictData::data("mixingPlane");
    }
  }
}




void MixingPlaneGGIBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  GGIBCBase::addIntoDictionaries(dictionaries);

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict mpd;
  mpd["default"]="areaAveraging";
  mpd["p"]="areaAveraging";
  mpd["U"]="areaAveraging";
  mpd["k"]="fluxAveraging";
  mpd["epsilon"]="fluxAveraging";
  mpd["omega"]="fluxAveraging";
  mpd["nuTilda"]="fluxAveraging";

  if (fvSchemes.find("mixingPlane")==fvSchemes.end())
    fvSchemes["mixingPlane"]=mpd;
}




} // namespace insight
