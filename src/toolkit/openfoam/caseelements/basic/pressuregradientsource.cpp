#include "pressuregradientsource.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(PressureGradientSource);
addToOpenFOAMCaseElementFactoryTable(PressureGradientSource);

PressureGradientSource::PressureGradientSource( OpenFOAMCase& c, const ParameterSet& ps )
: cellSetFvOption(c, "PressureGradientSource"+ps.getString("name"), ps),
  p_(ps)
{
}

void PressureGradientSource::addIntoFvOptionDictionary(
    OFDictData::dict& fvOptions,
    OFdicts& dictionaries ) const
{
  if (OFversion()>=220)
  {
    OFDictData::dict coeffs;
    OFDictData::list flds; flds.push_back("U");
    coeffs["Ubar"]=OFDictData::vector3(p_.Ubar);

    OFDictData::dict fod;
    if (OFversion()>=300)
    {
        fod["type"]="meanVelocityForce";
        coeffs["selectionMode"]="all";
        if (OFversion()>=400)
        {
            coeffs["fields"]=flds;
        }
        else
        {
            coeffs["fieldNames"]=flds;
        }
        fod["meanVelocityForceCoeffs"]=coeffs;
    }
    else
    {
        fod["type"]="pressureGradientExplicitSource";
        fod["selectionMode"]="all";
        coeffs["fieldNames"]=flds;
        fod["pressureGradientExplicitSourceCoeffs"]=coeffs;
    }

    fvOptions[name()]=fod;
    cellSetFvOption::addIntoFvOptionDictionary(fvOptions, dictionaries);
  }
  else
  {
    // for channelFoam:
    OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
    transportProperties["Ubar"]=OFDictData::dimensionedData("Ubar", dimVelocity, OFDictData::vector3(p_.Ubar));
  }
}


} // namespace insight
