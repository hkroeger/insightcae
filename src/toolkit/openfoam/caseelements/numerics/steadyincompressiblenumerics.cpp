#include "steadyincompressiblenumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {

defineType(steadyIncompressibleNumerics);
addToOpenFOAMCaseElementFactoryTable(steadyIncompressibleNumerics);


steadyIncompressibleNumerics::steadyIncompressibleNumerics(OpenFOAMCase& c, ParameterSetInput ip, const std::string& pName)
: FVNumerics(c, ip.forward<Parameters>(), pName)
{
  OFcase().addField(pName, FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({p().pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p().Uinternal.begin(), p().Uinternal.end()), volField ) );
}


void steadyIncompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  setApplicationName(dictionaries, "simpleFoam");


  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers[pName_]        =
      isGAMGOk() ?
        OFcase().GAMGPCGSolverSetup(1e-7, 0.01)
      :
        OFcase().stdSymmSolverSetup(1e-7, 0.01)
        ;
  solvers["U"]        = OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["k"]        = OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["R"]        = OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]    = OFcase().smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]  = OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]  = OFcase().smoothSolverSetup(1e-8, 0.1);

  setRelaxationFactors
  (
    dictionaries,
    {
      {"U",       0.7},
      {"k",       0.7},
      {"R",       0.7},
      {"omega",   0.7},
      {"epsilon", 0.7},
      {"nuTilda", 0.7}
    },
    {
      {pName_,       0.3}
    }
  );

  OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=0;
  SIMPLE["pRefCell"]=0;
  SIMPLE["pRefValue"]=p().pinternal;

  if ( (OFversion()>=210) && p().checkResiduals )
  {
    OFDictData::dict resCtrl;
    resCtrl["p"]=1e-4;
    resCtrl["U"]=1e-3;
    resCtrl["\"(k|epsilon|omega|nuTilda|R)\""]=1e-4;
    SIMPLE["residualControl"]=resCtrl;
  }

  // ============ setup fvSchemes ================================

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";


  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref, suf;
  if (OFversion()>=220) pref="bounded ";

  div["default"]="none";

  div["div(phi,U)"]	=	pref+"Gauss linearUpwindV "+gradNameOrScheme(dictionaries, "grad(U)");
  div["div(phi,nuTilda)"]       = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(nuTilda)");
  div["div(phi,k)"]		= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(k)");
  div["div(phi,epsilon)"]	= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(epsilon)");
  div["div(phi,omega)"]		= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(omega)");
  div["div(phi,R)"]             = "Gauss upwind";
  div["div(R)"]                 = "Gauss linear";

  div["div((nuEff*dev(grad(U).T())))"]="Gauss linear"; // kOmegaSST2

  if (OFversion()>=300)
  {
    div["div((nuEff*dev2(T(grad(U)))))"]="Gauss linear";
    div["div((nu*dev2(T(grad(U)))))"]="Gauss linear"; // LRR
  }
  else
  {
    div["div((nuEff*dev(T(grad(U)))))"]="Gauss linear";
  }

}


bool steadyIncompressibleNumerics::isCompressible() const
{
  return false;
}


} // namespace insight
