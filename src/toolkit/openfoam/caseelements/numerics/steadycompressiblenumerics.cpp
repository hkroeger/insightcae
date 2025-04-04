#include "steadycompressiblenumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(steadyCompressibleNumerics);
addToOpenFOAMCaseElementFactoryTable(steadyCompressibleNumerics);

steadyCompressibleNumerics::steadyCompressibleNumerics(OpenFOAMCase& c, ParameterSetInput ip)
: FVNumerics(c, ip.forward<Parameters>(), "p")
{
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure, 	FieldValue({p().pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p().Uinternal.begin(), p().Uinternal.end()), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature, 	FieldValue({p().Tinternal}), volField ) );
  OFcase().addField("alphat", FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({0.0}), volField ) );
}



void steadyCompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");

  setApplicationName(dictionaries, "rhoSimpleFoam");

  controlDict["endTime"]=p().endTime;


  // ============ setup fvSolution ================================
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& solvers=fvSolution.subDict("solvers");

  solvers["rho"]=OFcase().stdSymmSolverSetup(1e-7, 0.01);

  if (p().transonic)
    {
      solvers["p"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
    }
  else
    {
      solvers["p"]=
          isGAMGOk()
           ? OFcase().GAMGPCGSolverSetup(1e-8, 0.01)
           : OFcase().stdSymmSolverSetup(1e-8, 0.01);
    }

  solvers["U"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["e"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["h"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=OFcase().smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=OFcase().smoothSolverSetup(1e-8, 0.1);

  OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=p().nNonOrthogonalCorrectors;
  SIMPLE["rhoMin"]=p().rhoMin;
  SIMPLE["rhoMax"]=p().rhoMax;
  SIMPLE["consistent"]=p().consistent;
  SIMPLE["transonic"]=p().transonic;


  setRelaxationFactors
  (
    dictionaries,
    {
      {"U",       0.7},
      {"k",       0.7},
      {"R",       0.7},
      {"omega",   0.7},
      {"epsilon", 0.7},
      {"nuTilda", 0.7},
      {"e",       0.3},
      {"h",       0.3}
    },
    {
      {pName_,       0.3},
      {"rho",       0.01}
    }
  );

  // ============ setup fvSchemes ================================
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";


  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref, suf;
  if (OFversion()>=220) pref="bounded ";

  if (p().setup == Parameters::setup_type::accurate)
    {
      div["div(phi,U)"]       =	pref+"Gauss linearUpwindV "+gradNameOrScheme(dictionaries, "grad(U)");
      div["div(phi,e)"]       =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(e)");
      div["div(phi,h)"]       =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(h)");
      div["div(phi,K)"]       = pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(K)");
      div["div(phid,p)"]      = pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(p)");
      div["div(phi,k)"]       =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(k)");
      div["div(phi,epsilon)"] =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(epsilon)");
      div["div(phi,omega)"]   =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(omega)");
      div["div(phi,nuTilda)"] =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(nuTilda)");
    }
  else if (p().setup == Parameters::setup_type::stable)
    {
      div["div(phi,U)"]       =	pref+"Gauss upwind";
      div["div(phi,e)"]       =	pref+"Gauss upwind";
      div["div(phi,h)"]       =	pref+"Gauss upwind";
      div["div(phi,K)"]       = pref+"Gauss upwind";
      div["div(phid,p)"]      = pref+"Gauss upwind";
      div["div(phi,k)"]       =	pref+"Gauss upwind";
      div["div(phi,epsilon)"] =	pref+"Gauss upwind";
      div["div(phi,omega)"]   =	pref+"Gauss upwind";
      div["div(phi,nuTilda)"] =	pref+"Gauss upwind";
    }

  if (OFversion()>=230)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else if (OFversion()>=210)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else
  {
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";
  }

}

bool steadyCompressibleNumerics::isCompressible() const
{
  return true;
}


} // namespace insight
