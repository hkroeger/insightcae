#include "compressibleinletbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {




defineType(CompressibleInletBC);
addToFactoryTable(BoundaryCondition, CompressibleInletBC);
addToStaticFunctionTable(BoundaryCondition, CompressibleInletBC, defaultParameters);


CompressibleInletBC::CompressibleInletBC
(
    OpenFOAMCase& c,
    const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip
)
    : VelocityInletBC (
          c, patchName, boundaryDict,
          ip.forward<Parameters>() )
{
    BCtype_="patch";
}

void CompressibleInletBC::setField_p (
    OFDictData::dict& BC, OFdicts& dictionaries, bool isPrgh ) const
{
    BC["type"]=OFDictData::data ( "fixedValue" );
    BC["value"]=OFDictData::toUniformField(p().pressure);
}





} // namespace insight
