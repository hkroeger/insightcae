#ifndef THERMODYNAMICMODEL_H
#define THERMODYNAMICMODEL_H

#include "openfoam/caseelements/openfoamcaseelement.h"
#include "thermodynamicmodel_pdl.h"

namespace insight
{

class thermodynamicModel
: public OpenFOAMCaseElement
{
public:
  thermodynamicModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );

  static std::string category();
};


class thermophysicalModel
    : public thermodynamicModel
{

public:
#include THERMODYNAMICMODEL_PDL_thermophysicalModel
/*
PARAMETERSET>>>
inherits thermodynamicModel::Parameters

phaseName = string "" "Phase name for multi-phase turbulence models. When non-empty, field names and dictionary names get a '.<phaseName>' suffix (e.g. 'k.water', 'turbulenceProperties.water'). Leave empty for single-phase cases (backward-compatible default)."

createGetters
<<<PARAMETERSET
*/

    thermophysicalModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );

    /**
   * Returns the thermophysicalProperties dictionary path with the phase suffix
   * when a phaseName is set (e.g. "constant/thermophysicalProperties.water").
   * Returns "constant/thermophysicalProperties" for single-phase cases.
   */
    std::string thermophysicalPropertiesDictName() const;
};


}

#endif // THERMODYNAMICMODEL_H
