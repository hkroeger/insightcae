#ifndef INSIGHT_POROUSZONE_H
#define INSIGHT_POROUSZONE_H

#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/caseelements/basic/fvoption.h"

#include "porouszone__porousZoneConfig__Parameters_headers.h"

namespace insight {

class porousZoneConfig
{

public:
#include "porouszone__porousZoneConfig__Parameters.h"
/*
PARAMETERSET>>> porousZoneConfig Parameters

name = string "porosity" "Name of the porous cell zone. It needs to exist for this configuration to work."
d = vector (1 1 1) "Darcy coefficients for each direction"
f = vector (0 0 0) "Forchheimer coefficients for each direction"

direction_x = vector (1 0 0) "X direction of the porosity coordinate system"
direction_y = vector (0 1 0) "Y direction of the porosity coordinate system"
<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    porousZoneConfig ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDict ( OFDictData::dict& dict) const;
};




class porousZone
    : public OpenFOAMCaseElement,
      public porousZoneConfig
{

public:
    declareType ( "porousZone" );
    porousZone ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }
};




class porousZoneOption
    : public cellSetFvOption,
      public porousZoneConfig
{
public:
    typedef porousZoneConfig::Parameters Parameters;

public:
    declareType ( "porousZoneOption" );
    porousZoneOption ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoFvOptionDictionary ( OFDictData::dict& fvOptionDict, OFdicts& dictionaries ) const override;

    static std::string category() { return "Body Force"; }

    inline static insight::ParameterSet defaultParameters()
    { return Parameters::makeDefault(); }

};



} // namespace insight

#endif // INSIGHT_POROUSZONE_H
