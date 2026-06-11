#ifndef THERMODYNAMICMODEL_H
#define THERMODYNAMICMODEL_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight
{

class thermodynamicModel
: public OpenFOAMCaseElement
{
public:
  thermodynamicModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );

  static std::string category();
};

}

#endif // THERMODYNAMICMODEL_H
