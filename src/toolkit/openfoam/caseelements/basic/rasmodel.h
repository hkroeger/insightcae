#ifndef INSIGHT_RASMODEL_H
#define INSIGHT_RASMODEL_H

#include "openfoam/caseelements/turbulencemodel.h"

#include "rasmodel__RASModel__Parameters_headers.h"

namespace insight {

class RASModel
: public turbulenceModel
{

public:
#include "rasmodel__RASModel__Parameters.h"
/*
PARAMETERSET>>> RASModel Parameters
inherits turbulenceModel::Parameters

Prt = double 1.0 "[-] Turbulent Prandtl number"

createGetters
<<<PARAMETERSET
*/

public:
  declareType("RASModel");

  RASModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );

  /**
   * name of the OpenFOAM RAS model, written to RAS/RASModel
   * and used as prefix of the coefficient subdict "<name>Coeffs"
   */
  virtual std::string RASModelName() const;

  /**
   * contents of the "<RASModelName()>Coeffs" subdict
   */
  virtual OFDictData::dict RASModelCoeffs() const;

  void addIntoDictionaries(OFdicts& dictionaries) const override;
  OFDictData::dict& modelPropsDict(OFdicts& dictionaries) const override;
  AccuracyRequirement minAccuracyRequirement() const override;
};

} // namespace insight

#endif // INSIGHT_RASMODEL_H
