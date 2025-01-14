#ifndef INSIGHT_POTENTIALFREESURFACEBC_H
#define INSIGHT_POTENTIALFREESURFACEBC_H

#include "openfoam/caseelements/boundarycondition.h"


namespace insight {


class PotentialFreeSurfaceBC
: public BoundaryCondition
{
public:
  PotentialFreeSurfaceBC
  (
    OpenFOAMCase& c,
    const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip = ParameterSetInput()
  );
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};


} // namespace insight

#endif // INSIGHT_POTENTIALFREESURFACEBC_H
