#include "source.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(source);
addToOpenFOAMCaseElementFactoryTable(source);

source::source( OpenFOAMCase& c, ParameterSetInput ip)
: cellSetFvOption(c, /*"source"+ps.getString("name"),*/ ip.forward<Parameters>())
{}

void source::addIntoFvOptionDictionary(
    OFDictData::dict& fvOptions,
    OFdicts& od) const
{
  OFDictData::dict cd;
  cd["selectionMode"]="cellZone";
  cd["cellZone"]=p().zoneName;
  cd["volumeMode"]= p().volumeMode == Parameters::specific ? "specific" : "absolute";

  OFDictData::dict ijr;
  if (const auto* cs = boost::get<Parameters::value_scalar_type>(&p().value))
  {
    cd["type"]="scalarSemiImplicitSource";
    OFDictData::list vals;
    vals.push_back( cs->value_const );
    vals.push_back( cs->value_lin );
    ijr[p().fieldName]=vals;
  }
  else if (const auto* cs = boost::get<Parameters::value_vector_type>(&p().value))
  {
    cd["type"]="vectorSemiImplicitSource";
    OFDictData::list vals;
    vals.push_back( OFDictData::vector3(cs->value_const) );
    vals.push_back( cs->value_lin );
    ijr[p().fieldName]=vals;
  }

  cd["injectionRateSuSp"]=ijr;

  fvOptions[name()]=cd;
  cellSetFvOption::addIntoFvOptionDictionary(fvOptions, od);
}



} // namespace insight
