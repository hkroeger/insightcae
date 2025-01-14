#include "unsteadycompressiblenumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/basic/mrfzone.h"

namespace insight {


defineType(unsteadyCompressibleNumerics);
addToOpenFOAMCaseElementFactoryTable(unsteadyCompressibleNumerics);

unsteadyCompressibleNumerics::unsteadyCompressibleNumerics(OpenFOAMCase& c, ParameterSetInput ip)
: FVNumerics(c, ip.forward<Parameters>(), "p")
{
  OFcase().addField(pName_, FieldInfo(scalarField, 	dimPressure, 	FieldValue({p().pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p().Uinternal.begin(), p().Uinternal.end()), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature, 	FieldValue({p().Tinternal}), volField ) );
  OFcase().addField("alphat", FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({0.0}), volField ) );
}



void unsteadyCompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  bool LES=isLES();

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  if (p().formulation == Parameters::rhoPimpleFoam)
  {
    if (OFversion()<170)
    {
      if (OFcase().findElements<MRFZone>().size()>0)
        controlDict["application"]="rhoPorousMRFPimpleFoam";
      else
        controlDict["application"]="rhoPimpleFoam";
    }
    else
    {
      controlDict["application"]="rhoPimpleFoam";
    }
  }
  else if (p().formulation == Parameters::sonicFoam)
  {
    if (OFversion()<170 && (OFcase().findElements<MRFZone>().size()>0) )
    {
      controlDict["application"]="sonicMRFFoam";
    }
    else
    {
      controlDict["application"]="sonicFoam";
    }
  }


  CompressiblePIMPLESettings(p().time_integration).addIntoDictionaries(OFcase(), dictionaries);

  // ============ setup fvSolution ================================
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& solvers=fvSolution.subDict("solvers");

  double final_reltol_multiplier=0.0;
  if (const auto* simple = boost::get<Parameters::time_integration_type::pressure_velocity_coupling_SIMPLE_type>(&p().time_integration.pressure_velocity_coupling))
  {
      if (simple->relax_final)
        final_reltol_multiplier=1.0;
  }

  for ( auto s: std::map<std::string,double>({ {"", 1.0}, {"Final", final_reltol_multiplier} }) )
  {
    if (p().formulation == Parameters::sonicFoam)
    {
      solvers["rho"+s.first]=OFcase().diagonalSolverSetup();
    }
    else
    {
      solvers["rho"+s.first]=OFcase().stdSymmSolverSetup(1e-7, 0.1*s.second);
    }

    if (p().time_integration.transonic || (p().formulation == Parameters::sonicFoam) )
    {
      solvers["p"+s.first]=OFcase().stdAsymmSolverSetup(1e-8, 0.01*s.second);
    }
    else
    {
      solvers["p"+s.first]=
          isGAMGOk() ?
          OFcase().GAMGSolverSetup(1e-8, 0.01*s.second) :
          OFcase().stdSymmSolverSetup(1e-8, 0.01*s.second);
    }
    solvers["U"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["k"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["e"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["h"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["omega"+s.first]=OFcase().smoothSolverSetup(1e-12, 0.1*s.second, 1);
    solvers["epsilon"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["nuTilda"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
  }

  // ============ setup fvSchemes ================================
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  if (LES)
  {
//     ddt["default"]="CrankNicolson 0.75"; // problems with pressureGradientSource (oscillations), if coefficient is =1!
    ddt["default"]          = "backward"; // channel with Retau=180 gets laminar with CrankNicholson...
  }
  else
  {
    ddt["default"]          = "Euler";
  }


  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;

  div["div(phiU,p)"]        = "Gauss limitedLinear 1";
  div["div(phiv,p)"]        = "Gauss limitedLinear 1";
  div["div(phid,p)"]        = "Gauss limitedLinear 1";
  div["div(phi,K)"]         = "Gauss limitedLinear 1";


  if (LES)
  {
    /*if (OFversion()>=220)
      div["div(phi,U)"]="Gauss LUST grad(U)";
    else*/
    div["div(phi,U)"]       = "Gauss linear";
    div["div(phi,k)"]       = "Gauss linear";
    div["div(phi,e)"]       = "Gauss linear";
    div["div(phi,h)"]       = "Gauss linear";
    div["div(phi,K)"]       = "Gauss linear";
    div["div(phi,nuTilda)"] = "Gauss linear";
  }
  else
  {

    switch (p().setup)
    {
      case Parameters::setup_type::accurate:
        div["div(phi,U)"]       = "Gauss limitedLinearV 1";
        div["div(phi,k)"]       = "Gauss limitedLinear 1";
        div["div(phi,h)"]       = "Gauss limitedLinear 1";
        div["div(phi,e)"]       = "Gauss limitedLinear 1";
        div["div(phi,epsilon)"] = "Gauss limitedLinear 1";
        div["div(phi,omega)"]   = "Gauss limitedLinear 1";
        div["div(phi,nuTilda)"] = "Gauss limitedLinear 1";
        break;
      case Parameters::setup_type::medium:
        div["div(phi,U)"]       = "Gauss linearUpwindV "+gradNameOrScheme(dictionaries, "grad(U)");
        div["div(phi,k)"]       = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(k)");
        div["div(phi,h)"]       = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(h)");
        div["div(phi,e)"]       = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(e)");
        div["div(phi,epsilon)"] = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(epsilon)");
        div["div(phi,omega)"]   = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(omega)");
        div["div(phi,nuTilda)"] = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(nuTilda)");
        break;
      case Parameters::setup_type::stable:
        div["div(phi,U)"]       = "Gauss upwind";
        div["div(phi,k)"]       = "Gauss upwind";
        div["div(phi,h)"]       = "Gauss upwind";
        div["div(phi,e)"]       = "Gauss upwind";
        div["div(phi,epsilon)"] = "Gauss upwind";
        div["div(phi,omega)"]   = "Gauss upwind";
        div["div(phi,nuTilda)"] = "Gauss upwind";
        break;
    }
  }

  if (OFversion()>=230)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else if (OFversion()>=210)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else if (OFversion()>=164)
  {
    div["div((muEff*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else
  {
    div["div((muEff*dev2(grad(U).T())))"]="Gauss linear";
  }

}


bool unsteadyCompressibleNumerics::isCompressible() const
{
  return true;
}


} // namespace insight
