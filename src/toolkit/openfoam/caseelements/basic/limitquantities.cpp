#include "limitquantities.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(limitQuantities);
addToOpenFOAMCaseElementFactoryTable(limitQuantities);

limitQuantities::limitQuantities( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "", ps),
  p_(ps)
{
    name_="limitQuantities"+p_.name;
}

void limitQuantities::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& fvOptions=dictionaries.lookupDict("system/fvOptions");

  cellSetOption_Selection sel(p_.cells);

  if (const auto* limT = boost::get<Parameters::limitTemperature_limit_type>(&p_.limitTemperature))
    {
      OFDictData::dict cdT;
      cdT["type"]="limitTemperature";
      cdT["active"]=true;

      OFDictData::dict c;
      sel.insertSelection(c);
      c["min"]=limT->min;
      c["max"]=limT->max;
      cdT["limitTemperatureCoeffs"]=c;

      fvOptions[p_.name+"_temp"]=cdT;
    }

  if (const auto* limU = boost::get<Parameters::limitVelocity_limit_type>(&p_.limitVelocity))
    {
      OFDictData::dict cdU;
      cdU["type"]="limitVelocity";
      cdU["active"]=true;

      OFDictData::dict c;
      sel.insertSelection(c);
      c["max"]=limU->max;
      cdU["limitVelocityCoeffs"]=c;

      fvOptions[p_.name+"_vel"]=cdU;
    }

  for (const auto& i: p_.limitFields)
    {
      std::string type;
      switch (i.type)
        {
         case Parameters::limitFields_default_type::scalar:
           type="scalarlimitFields";
          break;
        }

      OFDictData::dict cd;
      cd["type"]=type;
      cd["active"]=true;

      OFDictData::dict c;
      sel.insertSelection(c);
      c["fieldName"]=i.fieldName;
      c["max"]=i.max;
      c["min"]=i.min;
      cd[type+"Coeffs"]=c;

      fvOptions[p_.name+"_"+i.fieldName]=cd;
    }

}



} // namespace insight
