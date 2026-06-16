#include "lrr_rasmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(LRR_RASModel);
addToFactoryTable(turbulenceModel, LRR_RASModel);
addToOpenFOAMCaseElementFactoryTable(LRR_RASModel);

void LRR_RASModel::addFields( OpenFOAMCase& c ) const
{
  c.addField(fieldName("nut"),     FieldInfo(scalarField,      dimKinViscosity,                  FieldValue({1e-10}), volField ) );
  c.addField(fieldName("k"),       FieldInfo(scalarField,      dimKinEnergy,                     FieldValue({1e-10}), volField ) );
  c.addField(fieldName("epsilon"), FieldInfo(scalarField,      OFDictData::dimension(0, 2, -3),  FieldValue({10.0}),  volField ) );
  c.addField(fieldName("R"),       FieldInfo(symmTensorField,  OFDictData::dimension(0, 2, -2),  FieldValue({1e-10,1e-10,1e-10,1e-10,1e-10,1e-10}), volField ) );
}

LRR_RASModel::LRR_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
: RASModel(c, ip.forward<Parameters>())
{
//   addFields();
}


void LRR_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
  RASProperties["RASModel"]="LRR";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.subDict("LRRCoeffs");
}

bool LRR_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
    if (fieldname == fieldName("k"))
    {
        BC["type"]=OFDictData::data("kqRWallFunction");
        BC["value"]=OFDictData::toUniformField(1e-10);
        return true;
    }
    else if (fieldname == fieldName("epsilon"))
    {
        BC["type"]=OFDictData::data("epsilonWallFunction");

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
    else if (fieldname == fieldName("R"))
    {
        BC["type"]=OFDictData::data("kqRWallFunction");
        BC["value"]=OFDictData::toUniformField(arma::ones(6,1)*1e-10);
            // OFDictData::data("uniform (1e-10 1e-10 1e-10 1e-10 1e-10 1e-10)");
        return true;
    }

    return false;
}



} // namespace insight
