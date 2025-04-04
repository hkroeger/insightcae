#include "fixedvalueconstraint.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(fixedValueConstraint);
addToOpenFOAMCaseElementFactoryTable(fixedValueConstraint);

fixedValueConstraint::fixedValueConstraint( OpenFOAMCase& c, ParameterSetInput ip)
    : cellSetFvOption(c, /*"fixedValueConstraint"+ps.getString("name"),*/ ip.forward<Parameters>())
{}

void fixedValueConstraint::addIntoFvOptionDictionary(
    OFDictData::dict& fvOptions,
    OFdicts& dictionaries ) const
{
  OFDictData::dict cd;
  cd["selectionMode"]="cellZone";
  cd["cellZone"]=p().zoneName;
  OFDictData::dict fvd;

  if (const auto* cs = boost::get<Parameters::value_scalar_type>(&p().value))
  {
    cd["type"]="scalarFixedValueConstraint";
    fvd[p().fieldName]=cs->value;
  }
  else if (const auto* cs = boost::get<Parameters::value_vector_type>(&p().value))
  {
    cd["type"]="vectorFixedValueConstraint";
    fvd[p().fieldName]=OFDictData::vector3(cs->value);
  }

  cd["fieldValues"]=fvd;

  fvOptions[p().name]=cd;

  cellSetFvOption::addIntoFvOptionDictionary(fvOptions, dictionaries);
}


} // namespace insight
