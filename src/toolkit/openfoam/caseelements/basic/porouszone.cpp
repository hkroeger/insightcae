#include "porouszone.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {




porousZoneConfig::porousZoneConfig( OpenFOAMCase& c, const ParameterSet& ps )
: p_(ps)
{}

void porousZoneConfig::addIntoDict(OFDictData::dict &pc) const
{
    pc["type"]="DarcyForchheimer";
    pc["selectionMode"]="cellZone";
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
}



defineType(porousZone);
addToOpenFOAMCaseElementFactoryTable(porousZone);


porousZone::porousZone( OpenFOAMCase& c, const ParameterSet& ps )
    : OpenFOAMCaseElement(c, "", ps),
    porousZoneConfig(c, ps)
{
    name_="porousZone"+p_.name;
}


void porousZone::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& porosityProperties
    = dictionaries.lookupDict("constant/porosityProperties");

  OFDictData::dict pc;
  addIntoDict(pc);
  porosityProperties[p_.name]=pc;

}




defineType(porousZoneOption);
addToOpenFOAMCaseElementFactoryTable(porousZoneOption);

porousZoneOption::porousZoneOption (
    OpenFOAMCase& c,
    const ParameterSet& ps
)
    : cellSetFvOption(
          c,
          "porousZoneOption"+ps.getString("name"),
          cellSetFvOption::Parameters() ),
      porousZoneConfig(c, ps)
{}


void porousZoneOption::addIntoFvOptionDictionary(
    OFDictData::dict& fvo,
    OFdicts& od) const
{
    OFDictData::dict  d;

    d["type"]="explicitPorositySource";

    OFDictData::dict cfg;
    addIntoDict(cfg);
    d["explicitPorositySourceCoeffs"]=cfg;

    fvo[name()]=d;

    cellSetFvOption::addIntoFvOptionDictionary(fvo, od);
}

} // namespace insight
