#include "cyclicggibc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {




defineType(CyclicGGIBC);
addToFactoryTable(BoundaryCondition, CyclicGGIBC);
addToStaticFunctionTable(BoundaryCondition, CyclicGGIBC, defaultParameters);




CyclicGGIBC::CyclicGGIBC(
    OpenFOAMCase& c, const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip )
: GGIBCBase(
          c, patchName, boundaryDict,
          ip.forward<Parameters>() )
{
}




void CyclicGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p().shadowPatch;
    bndDict["matchTolerance"]= 0.001;
    if (arma::norm(p().separationOffset,2)<1e-10)
    {
        bndDict["transform"]= "rotational";
    }
    else
    {
        bndDict["transform"]= "translational";
    }
    bndDict["rotationCentre"]=OFDictData::vector3(p().rotationCentre);
    bndDict["rotationAxis"]=OFDictData::vector3(p().rotationAxis);
    bndDict["separationVector"]=OFDictData::vector3(p().separationOffset);
    bndDict["rotationAngle"]=p().rotationAngle;
    bndDict["lowWeightCorrection"]=0.1;
  }
  else
  {
    bndDict["type"]="cyclicGgi";
    bndDict["shadowPatch"]= p().shadowPatch;
    bndDict["separationOffset"]=OFDictData::vector3(p().separationOffset);
    bndDict["bridgeOverlap"]=p().bridgeOverlap;
    bndDict["rotationAxis"]=OFDictData::vector3(p().rotationAxis);
    bndDict["rotationAngle"]=p().rotationAngle;
    bndDict["zone"]=p().zone;
  }
}




void CyclicGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
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
        BC["type"]=OFDictData::data("cyclicGgi");
    }
  }
}




} // namespace insight
