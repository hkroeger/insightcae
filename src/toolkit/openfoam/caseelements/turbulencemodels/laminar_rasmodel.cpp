#include "laminar_rasmodel.h"

namespace insight {


defineType(laminar_RASModel);
addToFactoryTable(turbulenceModel, laminar_RASModel);


addToOpenFOAMCaseElementFactoryTable(laminar_RASModel);

laminar_RASModel::laminar_RASModel(OpenFOAMCase& c, const ParameterSet& /*ps*/)
: RASModel(c)
{}


void laminar_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{

  if (OFversion()>=300)
   {
    OFDictData::dict& turbProperties=dictionaries.lookupDict("constant/turbulenceProperties");
    turbProperties["simulationType"]="laminar";
  }
  else{
    RASModel::addIntoDictionaries(dictionaries);
    OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
    RASProperties["RASModel"]="laminar";
    RASProperties["turbulence"]="true";
    RASProperties["printCoeffs"]="true";
    RASProperties.subDict("laminarCoeffs");
  }
}

bool laminar_RASModel::addIntoFieldDictionary(const std::string&, const FieldInfo&, OFDictData::dict&, double) const
{
  return false;
}



} // namespace insight
