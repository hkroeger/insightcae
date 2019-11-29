#include "potentialfreesurfacebc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {



PotentialFreeSurfaceBC::PotentialFreeSurfaceBC
(
  OpenFOAMCase& c,
  const std::string& patchName,
  const OFDictData::dict& boundaryDict,
  const ParameterSet& ps
)
: BoundaryCondition(c, patchName, boundaryDict, ps)
{
 BCtype_="patch";
}




void PotentialFreeSurfaceBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  //p_.phasefractions()->addIntoDictionaries(dictionaries);

  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);

    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("pressureInletOutletParSlipVelocity");
      BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
    }
    else if (
      (field.first=="T")
      &&
      (get<0>(field.second)==scalarField)
    )
    {
      BC["type"]="zeroGradient";
    }
    else if (
      ( (field.first=="p_gh") )
      &&
      (get<0>(field.second)==scalarField)
    )
    {
        BC["type"]=OFDictData::data("waveSurfacePressure");
        BC["value"]=OFDictData::data("uniform 0");
    }
    else if (
      ( (field.first=="p") )
      &&
      (get<0>(field.second)==scalarField)
    )
    {
        BC["type"]=OFDictData::data("zeroGradient");
        //BC["value"]=OFDictData::data("uniform 0");
    }
    else if
    (
      (
        (field.first=="k") ||
        (field.first=="epsilon") ||
        (field.first=="omega") ||
        (field.first=="nut") ||
        (field.first=="nuSgs") ||
        (field.first=="nuTilda")
      )
      &&
      (get<0>(field.second)==scalarField)
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    else
    {
      if (!(
          MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
/*	  ||
          p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)*/
          ))
        //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
        {
          BC["type"]=OFDictData::data("zeroGradient");
        }

    }
  }
}


} // namespace insight
