#ifndef INSIGHT_MRFZONE_H
#define INSIGHT_MRFZONE_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "mrfzone__MRFZone__Parameters_headers.h"


namespace insight {


class MRFZone
    : public OpenFOAMCaseElement
{

public:
#include "mrfzone__MRFZone__Parameters.h"
/*
PARAMETERSET>>> MRFZone Parameters
inherits OpenFOAMCaseElement::Parameters

name = string "rotor" "Name of the MRF zone"
rpm = double 1000.0 "Rotations per minute of the MRF zone"
nonRotatingPatches = array [ string "patchName" "Name of the patch to exclude from rotation" ] *0 "Name of patches to exclude from rotation"
rotationCentre = vector (0 0 0) "Base point of the rotation axis"
rotationAxis = vector (0 0 1) "Direction of the rotation axis"

createGetter
<<<PARAMETERSET
*/


public:
    declareType ( "MRFZone" );
    MRFZone ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Rotation"; }
};


} // namespace insight

#endif // INSIGHT_MRFZONE_H
