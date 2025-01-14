#include "simpledymfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {

defineType(simpleDyMFoamNumerics);
//addToOpenFOAMCaseElementFactoryTable(simpleDyMFoamNumerics);


simpleDyMFoamNumerics::simpleDyMFoamNumerics(OpenFOAMCase& c, ParameterSetInput ip)
: steadyIncompressibleNumerics(c, ip.forward<Parameters>())
{}


void simpleDyMFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  steadyIncompressibleNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  setApplicationName(dictionaries, "simpleDyMFoam");

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["writeInterval"]=OFDictData::data( p().FEMinterval );

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
  SIMPLE["startTime"]=OFDictData::data( 0.0 );
  SIMPLE["timeInterval"]=OFDictData::data( p().FEMinterval );

}

} // namespace insight
