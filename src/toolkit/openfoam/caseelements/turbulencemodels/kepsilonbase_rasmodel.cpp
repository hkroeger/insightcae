#include "kepsilonbase_rasmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


void kEpsilonBase_RASModel::addFields( OpenFOAMCase& c ) const
{
  c.addField(fieldName("k"),       FieldInfo(scalarField, dimKinEnergy,                   FieldValue({1e-10}), volField ) );
  c.addField(fieldName("epsilon"), FieldInfo(scalarField, OFDictData::dimension(0, 2, -3), FieldValue({10.0}),  volField ) );
  if (isCompressible())
  {
    c.addField(fieldName("mut"),   FieldInfo(scalarField, dimDynViscosity, FieldValue({1e-10}), volField ) );
    c.addField(fieldName("alphat"),FieldInfo(scalarField, dimDynViscosity, FieldValue({1e-10}), volField ) );
  }
  else
  {
    c.addField(fieldName("nut"),   FieldInfo(scalarField, dimKinViscosity, FieldValue({1e-10}), volField ) );
  }
}

kEpsilonBase_RASModel::kEpsilonBase_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
: RASModel(c, ip.forward<Parameters>())
{
//   addFields();
}


void kEpsilonBase_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
  RASProperties["RASModel"]=this->type(); //"kEpsilon";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties["kMin"]=1e-3;
  RASProperties["epsilonMin"]=1e-3;
  RASProperties.subDict(type()+"Coeffs");
}

bool kEpsilonBase_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
    std::string pref="";
    if (isCompressible()) pref="compressible::";

    if (fieldname == fieldName("k"))
    {
        BC["type"]=OFDictData::data(pref+"kqRWallFunction");
        BC["value"]=OFDictData::toUniformField(1e-10);
        return true;
    }
    else if (fieldname == fieldName("epsilon"))
    {
        BC["type"]=OFDictData::data(pref+"epsilonWallFunction");
        BC["value"]=OFDictData::toUniformField(10.);
        return true;
    }
    else if (fieldname == fieldName("nut"))
    {
        if (roughness_z0>0)
        {
            BC["type"]="nutkRoughWallFunction";
            double Cs=0.5;
            BC["Cs"]=Cs;
            BC["Ks"]=roughness_z0*9.793/Cs;
            BC["value"]=OFDictData::toUniformField(1e-10);
        }
        else
        {
            BC["type"]=OFDictData::data("nutkWallFunction");
            BC["value"]=OFDictData::toUniformField(1e-10);
        }
        return true;
    }
    else if (fieldname == fieldName("mut"))
    {
        BC["type"]=OFDictData::data("mutkWallFunction");
        BC["value"]=OFDictData::toUniformField(1e-10);
        return true;
    }
    else if (fieldname == fieldName("alphat"))
    {
        BC["type"]=OFDictData::data(pref+"alphatWallFunction");
        BC["value"]=OFDictData::toUniformField(1e-10);
        return true;
    }

    return false;
}



} // namespace insight
