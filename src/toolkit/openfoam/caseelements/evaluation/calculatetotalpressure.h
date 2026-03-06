#ifndef CALCULATETOTALPRESSURE_H
#define CALCULATETOTALPRESSURE_H

#include "openfoam/caseelements/analysiscaseelements.h"

#include "calculatetotalpressure__calculateTotalPressure__Parameters_headers.h"

namespace insight {

class calculateTotalPressure
    : public functionObject
{
public:
#include "calculatetotalpressure__calculateTotalPressure__Parameters.h"
/*
PARAMETERSET>>> calculateTotalPressure Parameters
inherits functionObject::Parameters

pAmbient = double 0. "Pressure value when pressure field has value zero"

pName = string "p" "Name of the pressure field"

rho = selectablesubset {{
 rhoInf set {
   density = double 1.4 "[kg/m^3] value of homogeneous density"
 }
 field set {
   rhoName = string "rho" "Name of the density field"
 }
}} field ""

UName = string "U" "Name of velocity field"

createGetter
<<<PARAMETERSET
*/

public:
    declareType("calculateTotalPressure");
    calculateTotalPressure(OpenFOAMCase &c, ParameterSetInput ip = Parameters());

    OFDictData::dict functionObjectDict() const override;
    std::set<std::string> requiredLibraries() const override;
};

} // namespace insight

#endif
