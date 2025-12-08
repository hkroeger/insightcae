#include "chtmultiregionnumerics.h"

#include "openfoam/caseelements/numerics/pimplesettings.h"
#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundaryconditions/wallbc.h"

namespace insight {


defineType(chtMultiRegionNumerics);
addToOpenFOAMCaseElementFactoryTable(chtMultiRegionNumerics);


void chtMultiRegionNumerics::init()
{
  if (OFversion() < 600)
    throw insight::UnsupportedFeature("chtMultiRegionNumerics currently supports only OF >=600");
}


chtMultiRegionNumerics::chtMultiRegionNumerics(OpenFOAMCase& c, ParameterSetInput ip)
    : FVNumerics(c, ip.forward<Parameters>(), "p_rgh")
{
    init();
}


void chtMultiRegionNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  insight::FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  if (auto us = unsteadyFormulation())
  {
    controlDict["application"]="chtMultiRegionFoam";

    CompressiblePIMPLESettings(us->time_integration)
        .addIntoDictionaries(OFcase(), dictionaries);
  }
  else
  {
    controlDict["application"]="chtMultiRegionSimpleFoam";
  }

  controlDict.getList("libs").insertNoDuplicate( "\"libwriteData.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"libconsistentCurveSampleSet.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalLimitedSnGrad.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"libnumericsFunctionObjects.so\"" );

  {
    OFDictData::dict wonow;
    wonow["type"]="writeData";
    wonow["fileName"]="\"wnow\"";
    wonow["fileNameAbort"]="\"wnowandstop\"";
    wonow["outputControl"]="timeStep";
    wonow["outputInterval"]=1;
    controlDict.subDict("functions")["writeData"]=wonow;
  }
  {
    OFDictData::dict fqmc;
    fqmc["type"]="faceQualityMarker";
    fqmc["lowerNonOrthThreshold"]=35.0;
    fqmc["upperNonOrthThreshold"]=60.0;
    controlDict.subDict("functions")["faceQualityMarker"]=fqmc;
  }

  // ============ setup regionProperties ================================

  OFDictData::dict& regionProperties=dictionaries.lookupDict("constant/regionProperties");

  OFDictData::list regions;
  regions.push_back("fluid");
  {
    OFDictData::list names;
    std::copy(p().fluidRegions.begin(), p().fluidRegions.end(), std::back_inserter(names));
    regions.push_back(names);
  }
  regions.push_back("solid");
  {
    OFDictData::list names;
    std::copy(p().solidRegions.begin(), p().solidRegions.end(), std::back_inserter(names));
    regions.push_back(names);
  }

  regionProperties["regions"]=regions;

}


bool chtMultiRegionNumerics::isCompressible() const
{
  return true;
}



bool chtMultiRegionNumerics::insertCoupledWall
(
    OpenFOAMCase& chc,
    const OFDictData::dict& boundaryDict,
    const std::string& patchName,
    const std::string& otherPatchName,
    const std::string& otherZoneName,
    double Tinitial,
    HeatBC::CHTCoupledWall::Parameters::offset_type offset )
{
    if (boundaryDict.find(patchName)!=boundaryDict.end())
    {
        WallBC::Parameters cwp;
        HeatBC::CHTCoupledWall::Parameters cp;
        cp.Tnbr="T";
        cp.samplePatch=otherPatchName;
        cp.sampleRegion=otherZoneName;
        cp.offset=offset;
        cp.Tinitial=Tinitial;
        cwp.heattransfer=std::make_shared<HeatBC::CHTCoupledWall>(cp);
        chc.insert(new WallBC(chc, patchName, boundaryDict, cwp));
        return true;
    }
    else
        return false;
}

const chtMultiRegionNumerics::Parameters::formulation_unsteady_type *
chtMultiRegionNumerics::unsteadyFormulation() const
{
    return boost::get<Parameters::formulation_unsteady_type>(&p().formulation);
};





defineType(chtMultiRegionFluidNumerics);
addToOpenFOAMCaseElementFactoryTable(chtMultiRegionFluidNumerics);


void chtMultiRegionFluidNumerics::init()
{
  if (OFversion() < 600)
    throw insight::UnsupportedFeature("chtMultiRegionNumerics currently supports only OF >=600");

  OFcase().addField("p_rgh", FieldInfo(scalarField, dimPressure,    FieldValue({p().pinternal}), volField ) );
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure,    FieldValue({p().pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 	FieldValue({p().Uinternal(0),p().Uinternal(1),p().Uinternal(2)} ), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature,	FieldValue({p().Tinternal}), volField ) );
}


chtMultiRegionFluidNumerics::chtMultiRegionFluidNumerics(OpenFOAMCase& c, ParameterSetInput ip)
: FVNumerics(c, ip.forward<Parameters>(), "p_rgh")
{
    init();
}


void chtMultiRegionFluidNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  insight::FVNumerics::addIntoDictionaries(dictionaries);


  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p_rgh"]=OFcase().stdSymmSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["\"(U|h|e|k|epsilon|nuTilda)\""]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["omega"]=OFcase().stdAsymmSolverSetup(1e-12, 0.01, 1);

  auto& parent = OFcase().parentRegion().findUniqueElement<chtMultiRegionNumerics>();
  if (auto ul = parent.unsteadyFormulation())
  {
      CompressiblePIMPLESettings(ul->time_integration)
          .addIntoDictionaries(OFcase(), dictionaries);

      OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
      ddt["default"]="Euler";

      solvers["p_rghFinal"]=OFcase().stdSymmSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.01);
      solvers["\"(U|h|e|k|epsilon|nuTilda)Final\""]=OFcase().stdAsymmSolverSetup(1e-8, 0.0);
      solvers["omegaFinal"]=OFcase().stdAsymmSolverSetup(1e-12, 0.0, 1);

  }
  else
  {
      OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
      {
        OFDictData::dict fieldRelax, eqnRelax;
        fieldRelax["rho"]=0.9;
        fieldRelax["p_rgh"]=0.7;

        eqnRelax["U"]=0.2;

        eqnRelax["h"]=
            eqnRelax["e"]=0.2;

        eqnRelax["epsilon"]=
            eqnRelax["nuTilda"]=
            eqnRelax["omega"]=
            eqnRelax["R"]=
            eqnRelax["k"]=0.7;

        relax["fields"]=fieldRelax;
        relax["equations"]=eqnRelax;
      }

      OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
      SIMPLE["nNonOrthogonalCorrectors"]=p().nNonOrthogonalCorrectors;
      SIMPLE["frozenFlow"]=p().frozenFlow;


      OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
      ddt["default"]="steadyState";
  }

  // ============ setup fvSchemes ================================

  insertStandardGradientConfig(dictionaries);

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref="bounded ";

  div["default"]="none";
  div["div(phi,U)"]	=	pref+"Gauss linearUpwindV limitedGrad";
  div["div(phi,h)"]	=	pref+"Gauss linearUpwind limitedGrad";
  div["div(phi,e)"]	=	pref+"Gauss linearUpwind limitedGrad";
  div["div(phi,K)"]	=	pref+"Gauss linearUpwind limitedGrad";

  div["div(phi,k)"]	=	pref+"Gauss linearUpwind grad(k)";
  div["div(phi,omega)"]	=	pref+"Gauss linearUpwind grad(omega)";
  div["div(phi,nuTilda)"]=	pref+"Gauss linearUpwind grad(nuTilda)";
  div["div(phi,epsilon)"]=	pref+"Gauss linearUpwind grad(epsilon)";
  div["div(phi,R)"]	=	pref+"Gauss upwind";
  div["div(R)"]="Gauss linear";

  div["div(((rho*nuEff)*dev(grad(U).T())))"]="Gauss linear"; // kOmegaSST2
  div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  div["div(((rho*nu)*dev2(T(grad(U)))))"]="Gauss linear"; // LRR

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear localLimited UBlendingFactor 1";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="localLimited UBlendingFactor 1";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p_rgh"]="";

}


bool chtMultiRegionFluidNumerics::isCompressible() const
{
  return true;
}







defineType(chtMultiRegionSolidNumerics);
addToOpenFOAMCaseElementFactoryTable(chtMultiRegionSolidNumerics);


void chtMultiRegionSolidNumerics::init()
{
    if (OFversion() < 600)
        throw insight::UnsupportedFeature("chtMultiRegionNumerics currently supports only OF >=600");

    OFcase().addField("p", FieldInfo(scalarField, 	dimPressure,		FieldValue({1e5}), volField ) );
    OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature,		FieldValue({p().Tinternal}), volField ) );
}


chtMultiRegionSolidNumerics::chtMultiRegionSolidNumerics(OpenFOAMCase& c, ParameterSetInput ip)
    : FVNumerics(c, ip.forward<Parameters>(), "p")
{
    init();
}

std::string chtMultiRegionSolidNumerics::lqGradSchemeIfPossible() const
{
    return "Gauss linear";
}


void chtMultiRegionSolidNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
    insight::FVNumerics::addIntoDictionaries(dictionaries);


    // ============ setup fvSolution ================================

    OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
    OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

    OFDictData::dict& solvers=fvSolution.subDict("solvers");
    solvers["h"]=OFcase().stdSymmSolverSetup(1e-8, 0.01);


    auto& parent = OFcase().parentRegion().findUniqueElement<chtMultiRegionNumerics>();
    if (auto ul = parent.unsteadyFormulation())
    {
        CompressiblePIMPLESettings(ul->time_integration)
        .addIntoDictionaries(OFcase(), dictionaries);

        OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
        ddt["default"]="Euler";

        solvers["hFinal"]=OFcase().stdSymmSolverSetup(1e-8, 0.0);
    }
    else
    {

        OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
        {
            OFDictData::dict eqnRelax;
            eqnRelax["h"]=0.7;
            relax["equations"]=eqnRelax;
        }

        OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
        SIMPLE["nNonOrthogonalCorrectors"]=p().nNonOrthogonalCorrectors;


        // ============ setup fvSchemes ================================


        OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
        ddt["default"]="steadyState";

    }


    insertStandardGradientConfig(dictionaries);

    OFDictData::dict& div=fvSchemes.subDict("divSchemes");
    div["default"]="none";

    OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
    laplacian["default"]="Gauss linear localLimited UBlendingFactor 1";

    OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
    interpolation["default"]="linear";

    OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
    snGrad["default"]="localLimited UBlendingFactor 1";

}


bool chtMultiRegionSolidNumerics::isCompressible() const
{
    return false;
}


} // namespace insight

