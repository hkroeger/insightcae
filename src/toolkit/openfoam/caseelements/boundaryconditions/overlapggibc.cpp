#include "overlapggibc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {




defineType(OverlapGGIBC);
addToFactoryTable(BoundaryCondition, OverlapGGIBC);
addToStaticFunctionTable(BoundaryCondition, OverlapGGIBC, defaultParameters);




OverlapGGIBC::OverlapGGIBC(
    OpenFOAMCase& c, const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip )
: GGIBCBase(
          c, patchName,
          boundaryDict,
          ip.forward<Parameters>() )
{}




void OverlapGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;

  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicPeriodicAMI";
    bndDict["neighbourPatch"]= p().shadowPatch;
    bndDict["matchTolerance"]= 0.001;
    bndDict["lowWeightCorrection"]=0.1;
    bndDict["maxIter"]=360;
    bndDict["periodicPatch"]= p().periodicPatch;
  }
  else
  {
    bndDict["type"]="overlapGgi";
    bndDict["shadowPatch"]= p().shadowPatch;
    bndDict["bridgeOverlap"]=p().bridgeOverlap;
    bndDict["separationOffset"]=OFDictData::vector3(p().separationOffset);
    bndDict["rotationAxis"]=OFDictData::vector3(p().rotationAxis);
    bndDict["nCopies"]=p().nCopies;
    bndDict["zone"]=p().zone;
  }
}




void OverlapGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
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
      if (OFversion()>=210)
        BC["type"]=OFDictData::data("cyclicAMI");
      else
        BC["type"]=OFDictData::data("overlapGgi");
    }
  }
}




} // namespace insight
