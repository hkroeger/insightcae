#ifndef INSIGHT_SETFIELDSCONFIGURATION_H
#define INSIGHT_SETFIELDSCONFIGURATION_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "setfieldsconfiguration__setFieldsConfiguration__Parameters_headers.h"

namespace insight {

class setFieldsConfiguration
    : public OpenFOAMCaseElement
{

public:
#include "setfieldsconfiguration__setFieldsConfiguration__Parameters.h"
/*
PARAMETERSET>>> setFieldsConfiguration Parameters

defaultValues = array [ selectablesubset {{

  scalar set {
   name = string "alpha.phase1" "Name of the field"
   value = double 0 "default value"
  }

  vector set {
   name = string "U" "Name of the field"
   value = vector (0 0 0) "default value"
  }

}} scalar ] *1 "default field values (in regions not covered by region selectors)"



regionSelectors = array [ selectablesubset {{

  box set {
   p0 = vector (-1e10 -1e10 -1e10) "Minimum corner of the box"
   p1 = vector (1e10 1e10 1e10) "Maximum corner of the box"

   selectfaces = bool true "check to select faces"
   selectcells = bool true "check to select cells"

   regionValues = array [ selectablesubset {{

      scalar set {
       name = string "alpha.phase1" "Name of the field"
       value = double 0 "field value"
      }

      vector set {
       name = string "U" "Name of the field"
       value = vector (0 0 0) "field value"
      }

    }} scalar ] *1 "field values in selected region"

  }

 }} box ]*1 "region selectors"


<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "setFieldsConfiguration" );
    setFieldsConfiguration ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    virtual bool isUnique() const;

    static std::string category() { return "Preprocessing"; }
};

} // namespace insight

#endif // INSIGHT_SETFIELDSCONFIGURATION_H
