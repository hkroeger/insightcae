#include "mirrormesh.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(mirrorMesh);
addToOpenFOAMCaseElementFactoryTable(mirrorMesh);

mirrorMesh::mirrorMesh( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "mirrorMesh", ps),
  p_(ps)
{
}

void mirrorMesh::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& mmd=dictionaries.lookupDict("system/mirrorMeshDict");

  mmd["planeTolerance"]=p_.planeTolerance;

  if (const Parameters::plane_pointAndNormal_type* pn =
      boost::get<Parameters::plane_pointAndNormal_type>(&p_.plane))
    {
      mmd["planeType"]="pointAndNormal";
      OFDictData::dict d;
      d["basePoint"]=OFDictData::vector3(pn->p0);
      d["normalVector"]=OFDictData::vector3(pn->normal);
      mmd["pointAndNormalDict"]=d;
    }
  else if (const Parameters::plane_threePoint_type* pt =
           boost::get<Parameters::plane_threePoint_type>(&p_.plane))
    {
      mmd["planeType"]="embeddedPoints";
      OFDictData::dict d;
      d["point1"]=OFDictData::vector3(pt->p0);
      d["point2"]=OFDictData::vector3(pt->p1);
      d["point3"]=OFDictData::vector3(pt->p2);
      mmd["embeddedPointsDict"]=d;
    }
  else throw insight::UnhandledSelection();
}

bool mirrorMesh::isUnique() const
{
  return true;
}

} // namespace insight
