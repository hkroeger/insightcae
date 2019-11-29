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
    const ParameterSet& ps
)
    : VelocityInletBC ( c, patchName, boundaryDict, ps ),
      ps_ ( ps )
{
    BCtype_="patch";
}

void CompressibleInletBC::setField_p ( OFDictData::dict& BC, OFdicts& dictionaries ) const
{
    BC["type"]=OFDictData::data ( "fixedValue" );
    BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( Parameters ( ps_ ).pressure ) );
}





} // namespace insight
