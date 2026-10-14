#include "komegasst_lowre_rasmodel.h"

using namespace boost;

namespace insight {


defineType(kOmegaSST_LowRe_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST_LowRe_RASModel);
addToOpenFOAMCaseElementFactoryTable(kOmegaSST_LowRe_RASModel);

kOmegaSST_LowRe_RASModel::kOmegaSST_LowRe_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
    : kOmegaSST_RASModel(c, ip.forward<Parameters>())
{}


std::string kOmegaSST_LowRe_RASModel::RASModelName() const
{
  return type();
}


OFDictData::dict kOmegaSST_LowRe_RASModel::RASModelCoeffs() const
{
  return OFDictData::dict();
}

bool kOmegaSST_LowRe_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
    if (roughness_z0>0.)
        throw insight::Exception("kOmegaSST_LowRe_RASModel: non-smooth walls are not supported!");

    if (fieldname == fieldName("k"))
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]=OFDictData::toUniformField(1e-10);
    return true;
  }
  else if (fieldname == fieldName("omega"))
  {
    BC["type"]=OFDictData::data("omegaWallFunction");
    BC["blended"]=true;
    BC["value"]=OFDictData::toUniformField(1e-10);
    return true;
  }
  return false;
}


} // namespace insight
