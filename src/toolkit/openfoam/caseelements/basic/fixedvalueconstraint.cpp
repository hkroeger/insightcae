#include "fixedvalueconstraint.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(fixedValueConstraint);
addToOpenFOAMCaseElementFactoryTable(fixedValueConstraint);

fixedValueConstraint::fixedValueConstraint( OpenFOAMCase& c, const ParameterSet& ps)
: OpenFOAMCaseElement(c, "", ps),
  p_(ps)
{
  name_="fixedValueConstraint"+p_.name;
}

void fixedValueConstraint::addIntoDictionaries ( OFdicts& dictionaries ) const
{
  OFDictData::dict cd;
  cd["active"]=true;
  cd["selectionMode"]="cellZone";
  cd["cellZone"]=p_.zoneName;
  OFDictData::dict fvd;

  if (const auto* cs = boost::get<Parameters::value_scalar_type>(&p_.value))
  {
    cd["type"]="scalarFixedValueConstraint";
    fvd[p_.fieldName]=cs->value;
  }
  else if (const auto* cs = boost::get<Parameters::value_vector_type>(&p_.value))
  {
    cd["type"]="vectorFixedValueConstraint";
    fvd[p_.fieldName]=OFDictData::vector3(cs->value);
  }

  cd["fieldValues"]=fvd;

  OFDictData::dict& fvOptions=dictionaries.lookupDict("system/fvOptions");
  fvOptions[p_.name]=cd;
}


} // namespace insight
