#ifndef INSIGHT_SIMPLEBC_H
#define INSIGHT_SIMPLEBC_H

#include <string>

#include "openfoam/caseelements/boundarycondition.h"

#include "simplebc__SimpleBC__Parameters_headers.h"

namespace insight {

class OpenFOAMCase;
class ParameterSet;
class OFdicts;

namespace OFDictData { class dict; }


class SimpleBC
: public BoundaryCondition
{

public:
#include "simplebc__SimpleBC__Parameters.h"
/*
PARAMETERSET>>> SimpleBC Parameters
inherits BoundaryCondition::Parameters

className = string "empty" "Class name of the boundary condition."

createGetter
<<<PARAMETERSET
*/

protected:

    void init();

public:
    declareType("SimpleBC");
  SimpleBC(
        OpenFOAMCase& c, const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const std::string className);

  SimpleBC(
      OpenFOAMCase& c, const std::string& patchName,
      const OFDictData::dict& boundaryDict,
      ParameterSetInput ip = ParameterSetInput() );

  void addIntoFieldDictionaries(OFdicts& dictionaries) const override;

};



} // namespace insight

#endif // INSIGHT_SIMPLEBC_H
