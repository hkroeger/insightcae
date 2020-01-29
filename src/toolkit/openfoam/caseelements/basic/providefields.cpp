#include "providefields.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"


namespace insight {

defineType(provideFields);
addToOpenFOAMCaseElementFactoryTable(provideFields);

provideFields::provideFields( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "provideFields", ps),
  p_(ps)
{
}

void provideFields::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& cd=dictionaries.lookupDict("system/controlDict");
  cd.getList("libs").insertNoDuplicate("\"libprovideFields.so\"");

  OFDictData::dict fod;
  fod["type"]="provideFields";

  OFDictData::list rf;
  fod["readFields"]=rf;

  OFDictData::dict csf;
  for (const auto& c: p_.createScalarFields)
  {
    OFDictData::dict sf;
    sf["dimensions"]=boost::str(boost::format("[%d %d %d %d 0]")
                                %c.dimensions[0]
                                %c.dimensions[1]
                                %c.dimensions[2]
                                %c.dimensions[3]);
    sf["value"]=c.scalarValue;
    csf[c.fieldName]=sf;
  }
  fod["createScalarFields"]=csf;

  cd.subDict("functions")["provideFields"] = fod;
}


std::string provideFields::category()
{
  return "PreProcessing";
}

bool provideFields::isUnique() const
{
  return true;
}

} // namespace insight
