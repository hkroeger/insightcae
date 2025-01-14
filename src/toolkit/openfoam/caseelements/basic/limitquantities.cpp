#include "limitquantities.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(limitQuantities);
addToOpenFOAMCaseElementFactoryTable(limitQuantities);

limitQuantities::limitQuantities( OpenFOAMCase& c, ParameterSetInput ip )
: OpenFOAMCaseElement(c, ip.forward<Parameters>())
{
    // name_="limitQuantities"+p().name;
}

void limitQuantities::addIntoDictionaries(OFdicts& dictionaries) const
{
  if (p().limitFields.size()>0)
  {
    dictionaries
        .lookupDict("system/controlDict")
        .getList("libs")
        .insertNoDuplicate("\"liblimitField.so\"")
        ;
  }

  OFDictData::dict& fvOptions=dictionaries.lookupDict("system/fvOptions");

  cellSetOption_Selection sel(p().cells);

  if (const auto* limT = boost::get<Parameters::limitTemperature_limit_type>(&p().limitTemperature))
    {
      OFDictData::dict cdT;
      cdT["type"]="limitTemperature";
      cdT["active"]=true;

      OFDictData::dict c;
      sel.insertSelection(c);
      c["min"]=limT->min;
      c["max"]=limT->max;
      cdT["limitTemperatureCoeffs"]=c;

      fvOptions[p().name+"_temp"]=cdT;
    }

  if (const auto* limU = boost::get<Parameters::limitVelocity_limit_type>(&p().limitVelocity))
    {
      OFDictData::dict cdU;
      cdU["type"]="limitVelocity";
      cdU["active"]=true;

      OFDictData::dict c;
      sel.insertSelection(c);
      c["max"]=limU->max;
      cdU["limitVelocityCoeffs"]=c;

      fvOptions[p().name+"_vel"]=cdU;
    }

  for (const auto& i: p().limitFields)
    {
      std::string type;
      switch (i.type)
        {
         case Parameters::limitFields_default_type::scalar:
           type="scalarlimitField";
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

      fvOptions[p().name+"_"+i.fieldName]=cd;
    }

}


} // namespace insight
