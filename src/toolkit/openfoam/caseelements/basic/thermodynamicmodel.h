#ifndef THERMODYNAMICMODEL_H
#define THERMODYNAMICMODEL_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight
{

class thermodynamicModel
: public OpenFOAMCaseElement
{
public:
  thermodynamicModel(OpenFOAMCase& c, const ParameterSet& ps);
};

}

#endif // THERMODYNAMICMODEL_H
