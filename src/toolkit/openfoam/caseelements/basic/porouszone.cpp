#include "porouszone.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(porousZone);
addToOpenFOAMCaseElementFactoryTable(porousZone);

porousZone::porousZone( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "", ps),
  p_(ps)
{
    name_="porousZone"+p_.name;
}

void porousZone::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& porosityProperties
    = dictionaries.lookupDict("constant/porosityProperties");

  OFDictData::dict pc;

  pc["type"]="DarcyForchheimer";
  pc["active"]=true;
  pc["cellZone"]=p_.name;
    OFDictData::dict dfc;
    dfc["d"]=OFDictData::vector3(p_.d);
    dfc["f"]=OFDictData::vector3(p_.f);

    OFDictData::dict cs;
     cs["type"]="cartesian";
     cs["origin"]=OFDictData::vector3(vec3(0,0,0));
     OFDictData::dict cr;
      cr["type"]="axesRotation";
      cr["e1"]=OFDictData::vector3(p_.direction_x);
      cr["e2"]=OFDictData::vector3(p_.direction_y);
     cs["coordinateRotation"]=cr;
    dfc["coordinateSystem"]=cs;
  pc["DarcyForchheimerCoeffs"]=dfc;

  porosityProperties[p_.name]=pc;

}


} // namespace insight
