#include "mrfzone.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(MRFZone);
addToOpenFOAMCaseElementFactoryTable(MRFZone);

MRFZone::MRFZone( OpenFOAMCase& c, ParameterSetInput ip )
: OpenFOAMCaseElement(c, ip.forward<Parameters>())
{
    // name_="MRFZone"+p_.name;
}

void MRFZone::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::list nrp; nrp.resize(p().nonRotatingPatches.size());
  copy(p().nonRotatingPatches.begin(), p().nonRotatingPatches.end(), nrp.begin());

  if (OFversion()<220)
  {
    OFDictData::dict coeffs;
    coeffs["nonRotatingPatches"]=nrp;
    coeffs["origin"]=OFDictData::dimensionedData(
        "origin", dimLength, OFDictData::vector3(p().rotationCentre)
    );
    coeffs["axis"]=OFDictData::dimensionedData(
        "axis", dimless, OFDictData::vector3(p().rotationAxis)
    );
    coeffs["omega"]=OFDictData::dimensionedData(
      "omega", OFDictData::dimension(0, 0, -1, 0, 0, 0, 0),
      2.*M_PI*p().rpm/60.
    );

    OFDictData::dict& MRFZones=dictionaries.lookupDict("constant/MRFZones");
    OFDictData::list& MRFZoneList = MRFZones.getList("");
    MRFZoneList.push_back(p().name);
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
    fod["origin"]=OFDictData::vector3(p().rotationCentre);
    fod["axis"]=OFDictData::vector3(p().rotationAxis);
    fod["omega"]=2.*M_PI*p().rpm/60.;

    fod["active"]=true;
    fod["selectionMode"]="cellZone";
    fod["cellZone"]=p().name;

    OFDictData::dict& MRFProps=dictionaries.lookupDict("constant/MRFProperties");
    MRFProps[p().name]=fod;

  }
  else
  {
    OFDictData::dict coeffs;

    coeffs["nonRotatingPatches"]=nrp;
    coeffs["origin"]=OFDictData::vector3(p().rotationCentre);
    coeffs["axis"]=OFDictData::vector3(p().rotationAxis);
    coeffs["omega"]=2.*M_PI*p().rpm/60.;

    OFDictData::dict fod;
    fod["type"]="MRFSource";
    fod["active"]=true;
    fod["selectionMode"]="cellZone";
    fod["cellZone"]=p().name;
    fod["MRFSourceCoeffs"]=coeffs;

    OFDictData::dict& fvOptions=dictionaries.lookupDict("system/fvOptions");
    fvOptions[p().name]=fod;
  }
}


} // namespace insight
