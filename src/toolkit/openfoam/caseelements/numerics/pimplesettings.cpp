#include "pimplesettings.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight
{




PIMPLESettings::PIMPLESettings(ParameterSetInput ip)
: p_(ip.forward<Parameters>())
{}


PIMPLESettings::~PIMPLESettings()
{}


void PIMPLESettings::addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");



  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  if (const auto* adj =
      boost::get<Parameters::timestep_control_adjust_type>(
          &p().timestep_control))
  {
    controlDict["adjustTimeStep"]=true;
    controlDict["maxCo"]=adj->maxCo;
    controlDict["maxDeltaT"]=adj->maxDeltaT;
  }
  else
  {
    controlDict["adjustTimeStep"]=false;
  }

  OFDictData::dict& PIMPLE=fvSolution.subDict("PIMPLE");
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");

  PIMPLE["nNonOrthogonalCorrectors"]=p().nonOrthogonalCorrectors;
  PIMPLE["momentumPredictor"]=p().momentumPredictor;

  if (const auto* pcp = boost::get<Parameters::pRefLocation_location_type>(&p().pRefLocation))
  {
    PIMPLE["pRefPoint"]=OFDictData::vector3(pcp->pRefPoint);
  }
  else if (const auto* pci = boost::get<Parameters::pRefLocation_cellID_type>(&p().pRefLocation))
  {
    PIMPLE["pRefCell"]=pci->pRefCell;
  }

  PIMPLE["pRefValue"]=p().pRefValue;

  if (const auto* piso = boost::get<Parameters::pressure_velocity_coupling_PISO_type>(&p().pressure_velocity_coupling))
  {
    PIMPLE["nCorrectors"]=piso->correctors;
    PIMPLE["nOuterCorrectors"]=1;
  }
  else if (const auto* simple = boost::get<Parameters::pressure_velocity_coupling_SIMPLE_type>(&p().pressure_velocity_coupling))
  {
    PIMPLE["nCorrectors"]=1;
    PIMPLE["nOuterCorrectors"]=simple->max_nOuterCorrectors;

    // SIMPLE mode: add underrelaxation
    OFDictData::dict& fieldRelax = oc.OFversion()<170 ? relax : relax.subDict("fields");
    OFDictData::dict& eqnRelax = oc.OFversion()<170 ? relax : relax.subDict("equations");

    eqnRelax["U"] = simple->relaxation_U;
    eqnRelax["UFinal"] = simple->relax_final ? double(simple->relaxation_U) : 1.0;
    for (const auto& fn: std::vector<std::string>({"p", "p_rgh", "p_gh", "pd"}))
    {
      fieldRelax[fn] = simple->relaxation_p;
      fieldRelax[fn+"Final"] = simple->relax_final ? double(simple->relaxation_p) : 1.0;
    }
    for (const auto& fn: std::vector<std::string>({"k", "epsilon", "omega", "nuTilda"}))
    {
      eqnRelax[fn] = simple->relaxation_turb;
      eqnRelax[fn+"Final"] = simple->relax_final ? double(simple->relaxation_turb) : 1.0;
    }
    for (const auto& fn: std::vector<std::string>({"T", "h", "e"}))
    {
      eqnRelax[fn] = simple->relaxation_e;
      eqnRelax[fn+"Final"] = simple->relax_final ? double(simple->relaxation_e) : 1.0;
    }

    // residual control
    OFDictData::dict& residualControl = PIMPLE.subDict("residualControl");

    if (simple->residual_p>1e-20)
    {
        OFDictData::dict tol_p;
        tol_p["tolerance"]=simple->residual_p;
        tol_p["relTol"]=0.0;
        residualControl["\"(p|p_rgh|pd).*\""] = tol_p;
    }

    if (simple->residual_U>1e-20)
    {
        OFDictData::dict tol_U;
        tol_U["tolerance"]=simple->residual_U;
        tol_U["relTol"]=0.0;
        residualControl["\"(U|k|epsilon|omega|nuTilda)\""] = tol_U;
    }

  }
  else if (const auto* pimple = boost::get<Parameters::pressure_velocity_coupling_PIMPLE_type>(&p().pressure_velocity_coupling))
  {
    PIMPLE["nCorrectors"]=pimple->nCorrectors;
    PIMPLE["nOuterCorrectors"]=pimple->max_nOuterCorrectors;

    // SIMPLE mode: add underrelaxation
    OFDictData::dict& fieldRelax = oc.OFversion()<170 ? relax : relax.subDict("fields");
    OFDictData::dict& eqnRelax = oc.OFversion()<170 ? relax : relax.subDict("equations");

    eqnRelax["U"] = pimple->relaxation_U;
    eqnRelax["UFinal"] = pimple->relax_final ? double(pimple->relaxation_U) : 1.0;
    for (const auto& fn: std::vector<std::string>({"p", "p_rgh", "p_gh", "pd"}))
    {
      fieldRelax[fn] = pimple->relaxation_p;
      fieldRelax[fn+"Final"] = pimple->relax_final ? double(pimple->relaxation_p) : 1.0;
    }
    for (const auto& fn: std::vector<std::string>({"k", "epsilon", "omega", "nuTilda"}))
    {
      eqnRelax[fn] = pimple->relaxation_turb;
      eqnRelax[fn+"Final"] = pimple->relax_final ? double(pimple->relaxation_turb) : 1.0;
    }
    for (const auto& fn: std::vector<std::string>({"T", "h", "e"}))
    {
      eqnRelax[fn] = pimple->relaxation_e;
      eqnRelax[fn+"Final"] = pimple->relax_final ? double(pimple->relaxation_e) : 1.0;
    }

    // residual control
    OFDictData::dict& residualControl = PIMPLE.subDict("residualControl");

    if (pimple->residual_p>1e-20)
    {
        OFDictData::dict tol_p;
        tol_p["tolerance"]=pimple->residual_p;
        tol_p["relTol"]=0.0;
        residualControl["\"(p|p_rgh|pd).*\""] = tol_p;
    }

    if (pimple->residual_U>1e-20)
    {
        OFDictData::dict tol_U;
        tol_U["tolerance"]=pimple->residual_U;
        tol_U["relTol"]=0.0;
        residualControl["\"(U|k|epsilon|omega|nuTilda)\""] = tol_U;
    }

  }
  else throw insight::UnhandledSelection();

}



bool PIMPLESettings::isPISO() const
{
  return boost::get<Parameters::pressure_velocity_coupling_PISO_type>(&p().pressure_velocity_coupling) != nullptr;
}

bool PIMPLESettings::isSIMPLE() const
{
  return boost::get<Parameters::pressure_velocity_coupling_SIMPLE_type>(&p().pressure_velocity_coupling) != nullptr;
}




CompressiblePIMPLESettings::CompressiblePIMPLESettings(ParameterSetInput ip)
    : PIMPLESettings(ip.forward<Parameters>())
{}



void CompressiblePIMPLESettings::addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const
{
  PIMPLESettings::addIntoDictionaries(oc, dictionaries);

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& PIMPLE=fvSolution.subDict("PIMPLE");
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");

  PIMPLE["transonic"]=p().transonic;
  PIMPLE["rhoMin"]=p().rhoMin;
  PIMPLE["rhoMax"]=p().rhoMax;
  if (oc.OFversion()>=655)
  {
    PIMPLE["pMin"]=p().pMin;
  }
  else
  {
    PIMPLE["pMin"]=OFDictData::dimensionedData("pMin", OFDictData::dimension(1, -1, -2), p().pMin);
  }



  // SIMPLE mode: add underrelaxation
  OFDictData::dict& fieldRelax = oc.OFversion()<170 ? relax : relax.subDict("fields");
  fieldRelax["rho"] = p().relaxation_rho;

}





MultiphasePIMPLESettings::MultiphasePIMPLESettings(ParameterSetInput ip)
: PIMPLESettings(ip.forward<Parameters>())
{}



void MultiphasePIMPLESettings::addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const
{
  PIMPLESettings::addIntoDictionaries(oc, dictionaries);

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  OFDictData::dict& eqnRelax = oc.OFversion()<170 ? relax : relax.subDict("equations");
  if (const auto* piso = boost::get<Parameters::pressure_velocity_coupling_PISO_type>(&p().pressure_velocity_coupling))
  {
    eqnRelax["\".*\""] = 1.0;
  }

  OFDictData::dict& controlDict = dictionaries.lookupDict("system/controlDict");
  controlDict["maxAlphaCo"]=p().maxAlphaCo;
}



}
