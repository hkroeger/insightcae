#include "pimplesettings.h"

namespace insight
{




PIMPLESettings::PIMPLESettings(const ParameterSet& ps)
: p_(ps)
{}



void PIMPLESettings::addIntoDictionaries ( OFdicts& dictionaries ) const
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
    OFDictData::dict& fieldRelax = relax.addSubDictIfNonexistent("fields");
    OFDictData::dict& eqnRelax = relax.addSubDictIfNonexistent("equations");

    fieldRelax["\".*Final\""] = 1.0;
    fieldRelax["\"(p|p_rgh|pd)\""] = simple->relaxation_p;

    eqnRelax["\".*Final\""] = 1.0;
    eqnRelax["U"] = simple->relaxation_U;
    eqnRelax["\"(k|epsilon|omega|nuTilda)\""] = simple->relaxation_turb;
    eqnRelax["\"(T|h|e)\""] = simple->relaxation_e;

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



void CompressiblePIMPLESettings::addIntoDictionaries ( OFdicts& dictionaries ) const
{
  PIMPLESettings::addIntoDictionaries(dictionaries);

  OFDictData::dict& fvSolution=dictionaries.addDictionaryIfNonexistent("system/fvSolution");
  OFDictData::dict& PIMPLE=fvSolution.addSubDictIfNonexistent("PIMPLE");
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");

  PIMPLE["transonic"]=p_.transonic;
  PIMPLE["rhoMin"]=p_.rhoMin;
  PIMPLE["rhoMax"]=p_.rhoMax;

  // SIMPLE mode: add underrelaxation
  OFDictData::dict& fieldRelax = relax.addSubDictIfNonexistent("fields");
  fieldRelax["rho"] = p_.relaxation_rho;

}





MultiphasePIMPLESettings::MultiphasePIMPLESettings(const ParameterSet& ps)
: PIMPLESettings(ps),
  p_(ps)
{}



void MultiphasePIMPLESettings::addIntoDictionaries ( OFdicts& dictionaries ) const
{
  PIMPLESettings::addIntoDictionaries(dictionaries);

  OFDictData::dict& controlDict = dictionaries.addDictionaryIfNonexistent("system/controlDict");

  controlDict["maxAlphaCo"]=p_.maxAlphaCo;
}



}
