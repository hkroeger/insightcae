#include "mrfzone.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(MRFZone);
addToOpenFOAMCaseElementFactoryTable(MRFZone);

MRFZone::MRFZone( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "", ps),
  p_(ps)
{
    name_="MRFZone"+p_.name;
}

void MRFZone::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::list nrp; nrp.resize(p_.nonRotatingPatches.size());
  copy(p_.nonRotatingPatches.begin(), p_.nonRotatingPatches.end(), nrp.begin());

  if (OFversion()<220)
  {
    OFDictData::dict coeffs;
    coeffs["nonRotatingPatches"]=nrp;
    coeffs["origin"]=OFDictData::dimensionedData(
      "origin", dimLength, OFDictData::vector3(p_.rotationCentre)
    );
    coeffs["axis"]=OFDictData::dimensionedData(
      "axis", dimless, OFDictData::vector3(p_.rotationAxis)
    );
    coeffs["omega"]=OFDictData::dimensionedData(
      "omega", OFDictData::dimension(0, 0, -1, 0, 0, 0, 0),
      2.*M_PI*p_.rpm/60.
    );

    OFDictData::dict& MRFZones=dictionaries.lookupDict("constant/MRFZones");
    OFDictData::list& MRFZoneList = MRFZones.getList("");
    MRFZoneList.push_back(p_.name);
    MRFZoneList.push_back(coeffs);

    OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
    if (controlDict.find("application")!=controlDict.end())
      if (controlDict.getString("application")=="simpleFoam")
        controlDict["application"]="MRFSimpleFoam";
  }
  else if (OFversion()>=300)
  {
    OFDictData::dict fod;

    fod["nonRotatingPatches"]=nrp;
    fod["origin"]=OFDictData::vector3(p_.rotationCentre);
    fod["axis"]=OFDictData::vector3(p_.rotationAxis);
    fod["omega"]=2.*M_PI*p_.rpm/60.;

    fod["active"]=true;
    fod["selectionMode"]="cellZone";
    fod["cellZone"]=p_.name;

    OFDictData::dict& MRFProps=dictionaries.lookupDict("constant/MRFProperties");
    MRFProps[p_.name]=fod;

  }
  else
  {
    OFDictData::dict coeffs;

    coeffs["nonRotatingPatches"]=nrp;
    coeffs["origin"]=OFDictData::vector3(p_.rotationCentre);
    coeffs["axis"]=OFDictData::vector3(p_.rotationAxis);
    coeffs["omega"]=2.*M_PI*p_.rpm/60.;

    OFDictData::dict fod;
    fod["type"]="MRFSource";
    fod["active"]=true;
    fod["selectionMode"]="cellZone";
    fod["cellZone"]=p_.name;
    fod["MRFSourceCoeffs"]=coeffs;

    OFDictData::dict& fvOptions=dictionaries.lookupDict("system/fvOptions");
    fvOptions[p_.name]=fod;
  }
}


} // namespace insight
