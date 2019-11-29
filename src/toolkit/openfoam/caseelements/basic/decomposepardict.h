#ifndef INSIGHT_DECOMPOSEPARDICT_H
#define INSIGHT_DECOMPOSEPARDICT_H


#include "openfoam/caseelements/openfoamcaseelement.h"


namespace insight {



/**
 * create a setDecomposeParDict
 * @poX,@poY,@poZ: define the preference of coordinate directions for decomposition
 * (relevant for methods simple and hierarchical).
 * If <0, direction will not be decomposed.
 * Number of decompositions along direction will be ordered according to value of po?
 */
class decomposeParDict
: public OpenFOAMCaseElement
{
public:
#include "decomposepardict__decomposeParDict__Parameters.h"
/*
PARAMETERSET>>> decomposeParDict Parameters

np = int 1 "Number of processors"

decompositionMethod = selection ( simple hierarchical metis scotch ) scotch "Parallel decomposition method"

decompWeights = vector (1 1 1) "Decomposition weights"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType ( "decomposeParDict" );
  decomposeParDict(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault());
  void addIntoDictionaries ( OFdicts& dictionaries ) const override;

  static std::string category();
};



} // namespace insight

#endif // INSIGHT_DECOMPOSEPARDICT_H
