#ifndef INSIGHT_WALLHEATFLUX_H
#define INSIGHT_WALLHEATFLUX_H

#include "openfoam/caseelements/analysiscaseelements.h"

#include "wallheatflux__wallHeatFlux__Parameters_headers.h"

namespace insight {

class wallHeatFlux
    : public outputFilterFunctionObject
{

public:
#include "wallheatflux__wallHeatFlux__Parameters.h"
/*
PARAMETERSET>>> wallHeatFlux Parameters
inherits outputFilterFunctionObject::Parameters

patches = array [ string "" "Name of a patch over which the heat flux shall be integrated. Regular expressions are recognized, if it is put in quotes." ] *1 "Patches"
region = string "region0" "Name of the region, for which the heat flux shall be determined."
qr = selectablesubset {{
 none set { }
 field set {
  fieldName = string "qr" "Name of the radiation heat flux field."
 }
}} none "Treatment of radiation heat flux"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "wallHeatFlux" );
    wallHeatFlux ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    OFDictData::dict functionObjectDict() const override;


    static std::map<std::string,arma::mat> readWallHeatFlux(
        const OpenFOAMCase& c,
        const boost::filesystem::path& location,
        const std::string& regionName,
        const std::string& foName );
};

} // namespace insight

#endif // INSIGHT_WALLHEATFLUX_H
