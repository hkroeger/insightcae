#ifndef INSIGHT_TURBULENCEMODEL_H
#define INSIGHT_TURBULENCEMODEL_H

#include "openfoam/ofdicts.h"
#include "openfoam/caseelements/openfoamcaseelement.h"

#include "turbulencemodel__turbulenceModel__Parameters_headers.h"

namespace insight {


namespace OFDictData
{
class dict;
}


class turbulenceModel
: public OpenFOAMCaseElement
{

public:
#include "turbulencemodel__turbulenceModel__Parameters.h"
/*
PARAMETERSET>>> turbulenceModel Parameters
inherits CaseElement::Parameters

phaseName = string "" "Phase name for multi-phase turbulence models. When non-empty, field names and dictionary names get a '.<phaseName>' suffix (e.g. 'k.water', 'turbulenceProperties.water'). Leave empty for single-phase cases (backward-compatible default)."

createGetters
<<<PARAMETERSET
*/

public:

  declareFactoryTable(
        turbulenceModel,
        LIST(OpenFOAMCase& ofc, ParameterSetInput&& ip ),
        LIST(ofc, std::move(ip))
        );

  enum AccuracyRequirement { AC_RANS, AC_LES, AC_DNS };

public:
  declareType("turbulenceModel");

  turbulenceModel(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );

  /**
   * Returns the field name with the phase suffix appended when a phaseName is
   * set (e.g. fieldName("k") returns "k.water" when phaseName is "water").
   * Returns @p baseName unchanged for single-phase cases (empty phaseName).
   */
  std::string fieldName(const std::string& baseName) const
  {
      const std::string& phase = p().phaseName;
      if (phase.empty()) return baseName;
      return baseName + "." + phase;
  }

  /**
   * Returns true if this turbulence model instance operates on a compressible phase.
   * Delegates to FVNumerics::isCompressible(phaseName) so the solver determines
   * compressibility — not a user-settable parameter. Falls back to the global
   * OpenFOAMCase::isCompressible() when phaseName is empty (backward-compatible).
   */
  bool isCompressible() const;

  /**
   * Returns the turbulenceProperties dictionary path with the phase suffix
   * when a phaseName is set (e.g. "constant/turbulenceProperties.water").
   * Returns "constant/turbulenceProperties" for single-phase cases.
   */
  std::string turbPropertiesDictName() const
  {
      const std::string& phase = p().phaseName;
      if (phase.empty()) return "constant/turbulenceProperties";
      return "constant/turbulenceProperties." + phase;
  }

  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const =0;
  virtual OFDictData::dict& modelPropsDict(OFdicts& dictionaries) const =0;

  virtual AccuracyRequirement minAccuracyRequirement() const =0;

  static std::string category() { return "Turbulence Model"; }

  static bool isInConflict(const CaseElement& e) { return (dynamic_cast<const turbulenceModel*>(&e)); }
};



} // namespace insight

#endif // INSIGHT_TURBULENCEMODEL_H
