#include "pimplesettings.h"

namespace insight
{




PIMPLESettings::PIMPLESettings(const ParameterSet& ps)
: p_(ps)
{}



void PIMPLESettings::addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const
{
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");



  OFDictData::dict& fvSolution=dictionaries.addDictionaryIfNonexistent("system/fvSolution");

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

  OFDictData::dict& PIMPLE=fvSolution.addSubDictIfNonexistent("PIMPLE");
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
    OFDictData::dict& fieldRelax = oc.OFversion()<170 ? relax : relax.addSubDictIfNonexistent("fields");
    OFDictData::dict& eqnRelax = oc.OFversion()<170 ? relax : relax.addSubDictIfNonexistent("equations");

    if (simple->relax_final)
    {
      fieldRelax["\"(p|p_rgh|pd).*\""] = simple->relaxation_p;
    }
    else
    {
      fieldRelax["\".*Final\""] = 1.0;
      fieldRelax["\"(p|p_rgh|pd)\""] = simple->relaxation_p;
    }

    if (simple->relax_final)
    {
      eqnRelax["\"U.*\""] = simple->relaxation_U;
      eqnRelax["\"(k|epsilon|omega|nuTilda).*\""] = simple->relaxation_turb;
      eqnRelax["\"(T|h|e).*\""] = simple->relaxation_e;
    }
    else
    {
      eqnRelax["\".*Final\""] = 1.0;
      eqnRelax["U"] = simple->relaxation_U;
      eqnRelax["\"(k|epsilon|omega|nuTilda)\""] = simple->relaxation_turb;
      eqnRelax["\"(T|h|e)\""] = simple->relaxation_e;
    }

    // residual control
    OFDictData::dict& residualControl = PIMPLE.addSubDictIfNonexistent("residualControl");

    OFDictData::dict tol_p;
    tol_p["tolerance"]=simple->residual_p;
    tol_p["relTol"]=0.0;
    residualControl["\"(p|p_rgh|pd).*\""] = tol_p;

    OFDictData::dict tol_U;
    tol_U["tolerance"]=simple->residual_U;
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

  OFDictData::dict& fvSolution=dictionaries.addDictionaryIfNonexistent("system/fvSolution");
  OFDictData::dict& PIMPLE=fvSolution.addSubDictIfNonexistent("PIMPLE");
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");

  PIMPLE["transonic"]=p_.transonic;
  PIMPLE["rhoMin"]=p_.rhoMin;
  PIMPLE["rhoMax"]=p_.rhoMax;
  PIMPLE["pMin"]=OFDictData::dimensionedData("pMin", OFDictData::dimension(1, -1, -2), p_.pMin);



  // SIMPLE mode: add underrelaxation
  OFDictData::dict& fieldRelax = oc.OFversion()<170 ? relax : relax.addSubDictIfNonexistent("fields");
  fieldRelax["rho"] = p_.relaxation_rho;

}





MultiphasePIMPLESettings::MultiphasePIMPLESettings(const ParameterSet& ps)
: PIMPLESettings(ps),
  p_(ps)
{}



void MultiphasePIMPLESettings::addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const
{
  PIMPLESettings::addIntoDictionaries(oc, dictionaries);

  OFDictData::dict& fvSolution=dictionaries.addDictionaryIfNonexistent("system/fvSolution");
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  OFDictData::dict& eqnRelax = oc.OFversion()<170 ? relax : relax.addSubDictIfNonexistent("equations");
  if (const auto* piso = boost::get<Parameters::pressure_velocity_coupling_PISO_type>(&p_.pressure_velocity_coupling))
  {
    eqnRelax["\".*\""] = 1.0;
  }

  OFDictData::dict& controlDict = dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict["maxAlphaCo"]=p_.maxAlphaCo;
}



}
