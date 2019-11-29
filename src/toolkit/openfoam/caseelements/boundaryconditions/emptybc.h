#ifndef INSIGHT_EMPTYBC_H
#define INSIGHT_EMPTYBC_H


#include "openfoam/caseelements/boundaryconditions/simplebc.h"

namespace insight {


class EmptyBC
: public SimpleBC
{

public:
  declareType("EmptyBC");
  EmptyBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& p = ParameterSet() );
  static ParameterSet defaultParameters() { return ParameterSet(); }
};


} // namespace insight

#endif // INSIGHT_EMPTYBC_H
