#ifndef INSIGHT_DISPLACEMENTFVMOTIONSOLVER_H
#define INSIGHT_DISPLACEMENTFVMOTIONSOLVER_H

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"

namespace insight {


class displacementFvMotionSolver
: public dynamicMesh
{
public:
  displacementFvMotionSolver(OpenFOAMCase& c);
  void addIntoDictionaries(OFdicts& dictionaries) const override;
};


} // namespace insight

#endif // INSIGHT_DISPLACEMENTFVMOTIONSOLVER_H
