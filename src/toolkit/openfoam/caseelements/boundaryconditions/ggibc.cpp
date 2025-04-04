#include "ggibc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {


defineType(GGIBC);
addToFactoryTable(BoundaryCondition, GGIBC);
addToStaticFunctionTable(BoundaryCondition, GGIBC, defaultParameters);

GGIBC::GGIBC(
    OpenFOAMCase& c, const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip )
: GGIBCBase(c, patchName, boundaryDict,
        ip.forward<Parameters>())
{
}

void GGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p().shadowPatch;
    bndDict["matchTolerance"]= 0.001;
    bndDict["lowWeightCorrection"]=0.1;
    //bndDict["transform"]= "rotational";
  }
  else
  {
    bndDict["type"]="ggi";
    bndDict["shadowPatch"]= p().shadowPatch;
    bndDict["separationOffset"]=OFDictData::vector3(p().separationOffset);
    bndDict["bridgeOverlap"]=p().bridgeOverlap;
    bndDict["zone"]=p().zone;
  }
}

void GGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
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
      if (OFversion()>=220)
        BC["type"]=OFDictData::data("cyclicAMI");
      else
        BC["type"]=OFDictData::data("ggi");
    }
  }
}


} // namespace insight
