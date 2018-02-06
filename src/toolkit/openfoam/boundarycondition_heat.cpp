
#include "openfoam/boundarycondition_heat.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{
namespace HeatBC
{

defineType(HeatBC);
defineDynamicClass(HeatBC);

HeatBC::~HeatBC()
{
}


void HeatBC::addIntoDictionaries(OFdicts&) const
{}




defineType(AdiabaticBC);
addToFactoryTable(HeatBC, AdiabaticBC);
addToStaticFunctionTable(HeatBC, AdiabaticBC, defaultParameters);

AdiabaticBC::AdiabaticBC(const ParameterSet& ps)
{}

bool AdiabaticBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
    if
    (
      (fieldname=="T")
      &&
      (get<0>(fieldinfo)==scalarField)
    )
    {
      BC["type"]="zeroGradient";
      return true;
    }
    else
      return false;
}





defineType(FixedTemperatureBC);
addToFactoryTable(HeatBC, FixedTemperatureBC);
addToStaticFunctionTable(HeatBC, FixedTemperatureBC, defaultParameters);

FixedTemperatureBC::FixedTemperatureBC(const ParameterSet& ps)
: p_(ps)
{}

bool FixedTemperatureBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
    if
    (
      (fieldname=="T")
      &&
      (get<0>(fieldinfo)==scalarField)
    )
    {
      FieldData(p_.T).setDirichletBC(BC);
      return true;
    }
    else
      return false;
}

}
}
