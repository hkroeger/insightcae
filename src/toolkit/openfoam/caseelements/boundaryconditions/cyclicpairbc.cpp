#include "cyclicpairbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundarycondition.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {


defineType(CyclicPairBC);
// addToFactoryTable(BoundaryCondition, CyclicPairBC);
// addToStaticFunctionTable(BoundaryCondition, CyclicPairBC, defaultParameters);

CyclicPairBC::CyclicPairBC(
    OpenFOAMCase& c, const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip )
: OpenFOAMCaseElement(c, /*patchName+"CyclicBC", */
                          ip.forward<Parameters>()),
  patchName_(patchName)
{
  if (c.OFversion()>=210)
  {
    nFaces_=boundaryDict.subDict(patchName_+"_half0").getInt("nFaces");
    startFace_=boundaryDict.subDict(patchName_+"_half0").getInt("startFace");
    nFaces1_=boundaryDict.subDict(patchName_+"_half1").getInt("nFaces");
    startFace1_=boundaryDict.subDict(patchName_+"_half1").getInt("startFace");
  }
  else
  {
    nFaces_=boundaryDict.subDict(patchName_).getInt("nFaces");
    startFace_=boundaryDict.subDict(patchName_).getInt("startFace");
  }
}


void CyclicPairBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  addIntoFieldDictionaries(dictionaries);

  OFDictData::dict bndsubd, bndsubd1;
  bndsubd["type"]="cyclic";
  bndsubd["nFaces"]=nFaces_;
  bndsubd["startFace"]=startFace_;
  bndsubd1["type"]="cyclic";
  bndsubd1["nFaces"]=nFaces1_;
  bndsubd1["startFace"]=startFace1_;

  if (OFversion()>=210)
  {
    bndsubd["neighbourPatch"]=patchName_+"_half1";
    bndsubd1["neighbourPatch"]=patchName_+"_half0";
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_+"_half0", bndsubd);
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_+"_half1", bndsubd1);
  }
  else
  {
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_, bndsubd);
  }
}

void CyclicPairBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second );
        OFDictData::dict& boundaryField=fieldDict.subDict ( "boundaryField" );

        if ( OFversion() >=210 ) {
            OFDictData::dict& BC=boundaryField.subDict ( patchName_+"_half0" );
            OFDictData::dict& BC1=boundaryField.subDict ( patchName_+"_half1" );

            if ( ( ( field.first=="motionU" ) || ( field.first=="pointDisplacement" ) ) ) {
                MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC );
                MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC1 );
            } else {
                BC["type"]="cyclic";
                BC1["type"]="cyclic";
            }
        } else {
            OFDictData::dict& BC=boundaryField.subDict ( patchName_ );

            if ( ( ( field.first=="motionU" ) || ( field.first=="pointDisplacement" ) ) ) {
                MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC );
            } else {
                BC["type"]="cyclic";
            }
        }
    }
}

bool CyclicPairBC::providesBCsForPatch(const std::string& patchName) const
{
  if (OFversion()>=210)
    return ( (patchName == patchName_+"_half0") || (patchName == patchName_+"_half1") );
  else
    return ( patchName == patchName_ );
}


} // namespace insight
