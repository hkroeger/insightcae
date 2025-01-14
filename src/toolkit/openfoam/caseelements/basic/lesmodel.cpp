#include "lesmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(LESModel);

LESModel::LESModel(OpenFOAMCase& c, ParameterSetInput ip)
: turbulenceModel(c, ip.forward<Parameters>())
{}


void LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
    OFDictData::dict& turbProperties=dictionaries.lookupDict("constant/turbulenceProperties");
    if (OFversion()>=300)
      turbProperties["simulationType"]="LES";
    else
      turbProperties["simulationType"]="LESModel";

    OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
    controlDict.getList("libs").insertNoDuplicate( "\"libnuSgsABLRoughWallFunction.so\"" );
}

OFDictData::dict& LESModel::modelPropsDict(OFdicts& dictionaries) const
{
  if (OFversion()>=300)
  {
      OFDictData::dict& turbProperties=dictionaries.lookupDict("constant/turbulenceProperties");
      OFDictData::dict& LESProperties = turbProperties.subDict("LES");
      LESProperties["turbulence"]=true;
      return LESProperties;
  }
  else
  {
      return dictionaries.lookupDict("constant/LESProperties");
  }
}

bool LESModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
    if (fieldname == "k")
    {
        BC["type"]="fixedValue";
        BC["value"]="uniform 1e-10";
        return true;
    }
    else if ( (fieldname == "nuSgs")||((OFversion()>=300)&&(fieldname == "nut")) )
    {
        if (roughness_z0>0.)
        {
//             std::cout<<"inserting \"nuSgsABLRoughWallFunction\" with z0="<<roughness_z0<<std::endl;
            BC["type"]="nuSgsABLRoughWallFunction";
            BC["z0"]=boost::str(boost::format("uniform %g")%roughness_z0);
            BC["value"]="uniform 1e-10";
        }
        else
        {
//             std::cout<<"not inserting since z0="<<roughness_z0<<std::endl;
            BC["type"]="zeroGradient";
        }
        return true;
    }

    return false;
}

turbulenceModel::AccuracyRequirement LESModel::minAccuracyRequirement() const
{
  return AC_LES;
}


} // namespace insight
