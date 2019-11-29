#ifndef INSIGHT_SYMMETRYBC_H
#define INSIGHT_SYMMETRYBC_H

#include "openfoam/caseelements/boundaryconditions/simplebc.h"

namespace insight {

class SymmetryBC
: public SimpleBC
{

public:
  declareType("SymmetryBC");
  SymmetryBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& p = ParameterSet() );
  static ParameterSet defaultParameters() { return ParameterSet(); }
};

} // namespace insight

#endif // INSIGHT_SYMMETRYBC_H
