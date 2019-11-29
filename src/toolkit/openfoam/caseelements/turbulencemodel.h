#ifndef INSIGHT_TURBULENCEMODEL_H
#define INSIGHT_TURBULENCEMODEL_H

#include "openfoam/ofdicts.h"
#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {


namespace OFDictData
{
class dict;
}


class turbulenceModel
: public OpenFOAMCaseElement
{


public:

  declareFactoryTable(turbulenceModel, LIST(OpenFOAMCase& ofc, const ParameterSet& ps = ParameterSet() ), LIST(ofc, ps));

  enum AccuracyRequirement { AC_RANS, AC_LES, AC_DNS };

public:
  declareType("turbulenceModel");

  turbulenceModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());

  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const =0;
  virtual OFDictData::dict& modelPropsDict(OFdicts& dictionaries) const =0;

  virtual AccuracyRequirement minAccuracyRequirement() const =0;

  static std::string category() { return "Turbulence Model"; }

  static bool isInConflict(const CaseElement& e) { return (dynamic_cast<const turbulenceModel*>(&e)); }
};



} // namespace insight

#endif // INSIGHT_TURBULENCEMODEL_H
