#ifndef INSIGHT_VELOCITYTETFEMMOTIONSOLVER_H
#define INSIGHT_VELOCITYTETFEMMOTIONSOLVER_H

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"
#include "openfoam/caseelements/numerics/tetfemnumerics.h"

namespace insight {


class velocityTetFEMMotionSolver
: public dynamicMesh
{
  tetFemNumerics tetFemNumerics_;

public:
  velocityTetFEMMotionSolver(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;
};


} // namespace insight

#endif // INSIGHT_VELOCITYTETFEMMOTIONSOLVER_H
