#include "kepsilonbase_rasmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


void kEpsilonBase_RASModel::addFields( OpenFOAMCase& c ) const
{
  c.addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	FieldValue({1e-10}), volField ) );
  c.addField("epsilon", 	FieldInfo(scalarField, 	OFDictData::dimension(0, 2, -3), 	FieldValue({10.0}), volField ) );
  if (c.isCompressible())
  {
    c.addField("mut", 	FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({1e-10}), volField ) );
    c.addField("alphat", 	FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({1e-10}), volField ) );
  }
  else
  {
    c.addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
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
    if (OFcase().isCompressible()) pref="compressible::";

    if (fieldname == "k")
    {
        BC["type"]=OFDictData::data(pref+"kqRWallFunction");
        BC["value"]=OFDictData::toUniformField(1e-10);
        return true;
    }
    else if (fieldname == "epsilon")
    {
        BC["type"]=OFDictData::data(pref+"epsilonWallFunction");
        BC["value"]=OFDictData::toUniformField(10.);
        return true;
    }
    else if (fieldname == "nut")
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
    else if (fieldname == "mut")
    {
        BC["type"]=OFDictData::data("mutkWallFunction");
        BC["value"]=OFDictData::toUniformField(1e-10);
        return true;
    }
    else if (fieldname == "alphat")
    {
        BC["type"]=OFDictData::data(pref+"alphatWallFunction");
        BC["value"]=OFDictData::toUniformField(1e-10);
        return true;
    }

    return false;
}



} // namespace insight
