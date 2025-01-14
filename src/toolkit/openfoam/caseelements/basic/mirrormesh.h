#ifndef INSIGHT_MIRRORMESH_H
#define INSIGHT_MIRRORMESH_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "mirrormesh__mirrorMesh__Parameters_headers.h"

namespace insight {


class mirrorMesh
    : public OpenFOAMCaseElement
{

public:
#include "mirrormesh__mirrorMesh__Parameters.h"
/*
PARAMETERSET>>> mirrorMesh Parameters
inherits OpenFOAMCaseElement::Parameters

plane = selectablesubset {{
 pointAndNormal set {
  p0 = vector (0 0 0) "Origin point"
  normal = vector (0 0 1) "plane normal"
 }

 threePoint set {
  p0 = vector (0 0 0) "First point"
  p1 = vector (1 0 0) "Second point"
  p2 = vector (0 1 0) "Third point"
 }

 }} pointAndNormal "Mirror plane definition"

planeTolerance = double 1e-3 "plane tolerance"

createGetter
<<<PARAMETERSET
*/


public:
    declareType ( "mirrorMesh" );
    mirrorMesh ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    virtual bool isUnique() const;

    static std::string category() { return "Meshing"; }
};


} // namespace insight

#endif // INSIGHT_MIRRORMESH_H
