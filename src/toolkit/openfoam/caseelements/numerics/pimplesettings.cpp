#include "pimplesettings.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight
{




PIMPLESettings::PIMPLESettings(const ParameterSet& ps)
: p_(ps)
{}


PIMPLESettings::~PIMPLESettings()
{}


void PIMPLESettings::addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");



  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  if (const auto* adj = boost::get<Parameters::timestep_control_adjust_type>(&p_.timestep_control))
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

  PIMPLE["nNonOrthogonalCorrectors"]=p_.nonOrthogonalCorrectors;
  PIMPLE["momentumPredictor"]=p_.momentumPredictor;

  PIMPLE["pRefCell"]=0;
#warning need proper pressure value here!
  PIMPLE["pRefValue"]=0.0;

  if (const auto* piso = boost::get<Parameters::pressure_velocity_coupling_PISO_type>(&p_.pressure_velocity_coupling))
  {
    PIMPLE["nCorrectors"]=piso->correctors;
    PIMPLE["nOuterCorrectors"]=1;
  }
  else if (const auto* simple = boost::get<Parameters::pressure_velocity_coupling_SIMPLE_type>(&p_.pressure_velocity_coupling))
  {
    PIMPLE["nCorrectors"]=1;
    PIMPLE["nOuterCorrectors"]=simple->max_nOuterCorrectors;

    // SIMPLE mode: add underrelaxation
    OFDictData::dict& fieldRelax = oc.OFversion()<170 ? relax : relax.subDict("fields");
    OFDictData::dict& eqnRelax = oc.OFversion()<170 ? relax : relax.subDict("equations");

    eqnRelax["U"] = simple->relaxation_U;
    eqnRelax["UFinal"] = simple->relax_final ? simple->relaxation_U : 1.0;
    for (const auto& fn: std::vector<std::string>({"p", "p_rgh", "p_gh", "pd"}))
    {
      fieldRelax[fn] = simple->relaxation_p;
      fieldRelax[fn+"Final"] = simple->relax_final ? simple->relaxation_p : 1.0;
    }
    for (const auto& fn: std::vector<std::string>({"k", "epsilon", "omega", "nuTilda"}))
    {
      eqnRelax[fn] = simple->relaxation_turb;
      eqnRelax[fn+"Final"] = simple->relax_final ? simple->relaxation_turb : 1.0;
    }
    for (const auto& fn: std::vector<std::string>({"T", "h", "e"}))
    {
      eqnRelax[fn] = simple->relaxation_e;
      eqnRelax[fn+"Final"] = simple->relax_final ? simple->relaxation_e : 1.0;
    }

    // residual control
    OFDictData::dict& residualControl = PIMPLE.subDict("residualControl");

    OFDictData::dict tol_p;
    tol_p["tolerance"]=simple->residual_p;
    tol_p["relTol"]=0.0;
    residualControl["\"(p|p_rgh|pd).*\""] = tol_p;

    OFDictData::dict tol_U;
    tol_U["tolerance"]=simple->residual_U;
    tol_U["relTol"]=0.0;
    residualControl["\"(U|k|epsilon|omega|nuTilda)\""] = tol_U;

  }
  else if (const auto* pimple = boost::get<Parameters::pressure_velocity_coupling_PIMPLE_type>(&p_.pressure_velocity_coupling))
  {
    PIMPLE["nCorrectors"]=pimple->nCorrectors;
    PIMPLE["nOuterCorrectors"]=pimple->max_nOuterCorrectors;

    // SIMPLE mode: add underrelaxation
    OFDictData::dict& fieldRelax = oc.OFversion()<170 ? relax : relax.subDict("fields");
    OFDictData::dict& eqnRelax = oc.OFversion()<170 ? relax : relax.subDict("equations");

    eqnRelax["U"] = pimple->relaxation_U;
    eqnRelax["UFinal"] = pimple->relax_final ? pimple->relaxation_U : 1.0;
    for (const auto& fn: std::vector<std::string>({"p", "p_rgh", "p_gh", "pd"}))
    {
      fieldRelax[fn] = pimple->relaxation_p;
      fieldRelax[fn+"Final"] = pimple->relax_final ? pimple->relaxation_p : 1.0;
    }
    for (const auto& fn: std::vector<std::string>({"k", "epsilon", "omega", "nuTilda"}))
    {
      eqnRelax[fn] = pimple->relaxation_turb;
      eqnRelax[fn+"Final"] = pimple->relax_final ? pimple->relaxation_turb : 1.0;
    }
    for (const auto& fn: std::vector<std::string>({"T", "h", "e"}))
    {
      eqnRelax[fn] = pimple->relaxation_e;
      eqnRelax[fn+"Final"] = pimple->relax_final ? pimple->relaxation_e : 1.0;
    }

    // residual control
    OFDictData::dict& residualControl = PIMPLE.subDict("residualControl");

    OFDictData::dict tol_p;
    tol_p["tolerance"]=pimple->residual_p;
    tol_p["relTol"]=0.0;
    residualControl["\"(p|p_rgh|pd).*\""] = tol_p;

    OFDictData::dict tol_U;
    tol_U["tolerance"]=pimple->residual_U;
    tol_U["relTol"]=0.0;
    residualControl["\"(U|k|epsilon|omega|nuTilda)\""] = tol_U;

  }
  else
  {
    throw insight::Exception("Internal error: unhandled selection!");
  }

}



bool PIMPLESettings::isPISO() const
{
  return boost::get<Parameters::pressure_velocity_coupling_PISO_type>(&p_.pressure_velocity_coupling) != nullptr;
}

bool PIMPLESettings::isSIMPLE() const
{
  return boost::get<Parameters::pressure_velocity_coupling_SIMPLE_type>(&p_.pressure_velocity_coupling) != nullptr;
}




CompressiblePIMPLESettings::CompressiblePIMPLESettings(const ParameterSet& ps)
: PIMPLESettings(ps),
  p_(ps)
{}



void CompressiblePIMPLESettings::addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const
{
  PIMPLESettings::addIntoDictionaries(oc, dictionaries);

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& PIMPLE=fvSolution.subDict("PIMPLE");
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");

  PIMPLE["transonic"]=p_.transonic;
  PIMPLE["rhoMin"]=p_.rhoMin;
  PIMPLE["rhoMax"]=p_.rhoMax;
  PIMPLE["pMin"]=OFDictData::dimensionedData("pMin", OFDictData::dimension(1, -1, -2), p_.pMin);



  // SIMPLE mode: add underrelaxation
  OFDictData::dict& fieldRelax = oc.OFversion()<170 ? relax : relax.subDict("fields");
  fieldRelax["rho"] = p_.relaxation_rho;

}





MultiphasePIMPLESettings::MultiphasePIMPLESettings(const ParameterSet& ps)
: PIMPLESettings(ps),
  p_(ps)
{}



void MultiphasePIMPLESettings::addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const
{
  PIMPLESettings::addIntoDictionaries(oc, dictionaries);

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  OFDictData::dict& eqnRelax = oc.OFversion()<170 ? relax : relax.subDict("equations");
  if (const auto* piso = boost::get<Parameters::pressure_velocity_coupling_PISO_type>(&p_.pressure_velocity_coupling))
  {
    eqnRelax["\".*\""] = 1.0;
  }

  OFDictData::dict& controlDict = dictionaries.lookupDict("system/controlDict");
  controlDict["maxAlphaCo"]=p_.maxAlphaCo;
}



}
