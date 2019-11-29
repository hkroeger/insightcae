#include "potentialfreesurfacefoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"

namespace insight {

defineType(potentialFreeSurfaceFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(potentialFreeSurfaceFoamNumerics);


potentialFreeSurfaceFoamNumerics::potentialFreeSurfaceFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: unsteadyIncompressibleNumerics(c, ps, "p_gh"),
  p_(ps)
{
  OFcase().addField("p", FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({0.0}), volField ) );
//  OFcase().addField("p_gh", FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({0.0}), volField ) );
//  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0.0,0.0,0.0}), volField ) );

  if (OFversion()<230)
    throw insight::Exception("solver potentialFreeSurfaceFoam not available in selected OpenFOAM version!");
}


void potentialFreeSurfaceFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  unsteadyIncompressibleNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  if (OFcase().findElements<dynamicMesh>().size()>0)
   setApplicationName(dictionaries, "potentialFreeSurfaceDyMFoam");
  else
   setApplicationName(dictionaries, "potentialFreeSurfaceFoam");

}


bool potentialFreeSurfaceFoamNumerics::isCompressible() const
{
  return false;
}

} // namespace insight
