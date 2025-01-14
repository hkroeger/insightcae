#include "unsteadyincompressiblenumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"

namespace insight {

defineType(unsteadyIncompressibleNumerics);
addToOpenFOAMCaseElementFactoryTable(unsteadyIncompressibleNumerics);

unsteadyIncompressibleNumerics::unsteadyIncompressibleNumerics(OpenFOAMCase& c, ParameterSetInput ip, const std::string& pName)
: FVNumerics(c, ip.forward<Parameters>(), pName)
{
  OFcase().addField(pName_, FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({p().pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p().Uinternal.begin(), p().Uinternal.end()), volField ) );
}



void unsteadyIncompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // check if LES required
  bool LES=isLES();

  // ============ setup controlDict ================================

  if ( (OFcase().findElements<dynamicMesh>().size()>0) && (OFversion()<600) )
   setApplicationName(dictionaries, "pimpleDyMFoam");
  else
   setApplicationName(dictionaries, "pimpleFoam");

  PIMPLESettings(p().time_integration).addIntoDictionaries(OFcase(), dictionaries);

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers[pName_]=isGAMGOk() ? OFcase().GAMGSolverSetup(1e-8, 0.01) : OFcase().stdSymmSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["pcorr"]=OFcase().stdSymmSolverSetup(1e-3, 0.01);
  solvers["U"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=OFcase().smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=OFcase().smoothSolverSetup(1e-8, 0.1);

  solvers[pName_+"Final"]=isGAMGOk() ? OFcase().GAMGPCGSolverSetup(1e-8, 0.0) : OFcase().stdSymmSolverSetup(1e-8, 0.0); //GAMGSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["pcorrFinal"]=OFcase().stdSymmSolverSetup(1e-3, 0.0);
  solvers["UFinal"]=OFcase().smoothSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=OFcase().smoothSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=OFcase().smoothSolverSetup(1e-14, 0, 1);
  solvers["epsilonFinal"]=OFcase().smoothSolverSetup(1e-8, 0);
  solvers["nuTildaFinal"]=OFcase().smoothSolverSetup(1e-8, 0);


  // ============ setup fvSchemes ================================


  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  if (LES)
  {
//     ddt["default"]="CrankNicolson 0.75"; // problems with pressureGradientSource (oscillations), if coefficient is =1!
    ddt["default"]="backward"; // channel with Retau=180 gets laminar with CrankNicholson...
  }
  else
  {
    ddt["default"]="Euler";
  }


  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  div["default"]="Gauss linear";


  if (LES)
  {
    /*if (OFversion()>=220)
      div["div(phi,U)"]="Gauss LUST grad(U)";
    else*/
    if (p().LESfilteredConvection)
      div["div(phi,U)"]="Gauss filteredLinear";
    else
      div["div(phi,U)"]="Gauss linear";
    div["div(phi,k)"]="Gauss linear";
    div["div(phi,nuTilda)"]="Gauss linear";
  }
  else
  {
    div["div(phi,U)"]="Gauss limitedLinearV 1";
    div["div(phi,k)"]="Gauss limitedLinear 1";
    div["div(phi,epsilon)"]="Gauss limitedLinear 1";
    div["div(phi,omega)"]="Gauss limitedLinear 1";
    div["div(phi,nuTilda)"]="Gauss limitedLinear 1";
  }

  if (OFversion()>=230)
  {
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";
  }
  else if (OFversion()>=210)
  {
    div["div((nuEff*dev(T(grad(U)))))"]="Gauss linear";
  }
  else
  {
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";
  }

}


bool unsteadyIncompressibleNumerics::isCompressible() const
{
  return false;
}

} // namespace insight
