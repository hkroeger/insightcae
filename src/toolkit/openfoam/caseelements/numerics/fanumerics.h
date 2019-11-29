#ifndef INSIGHT_FANUMERICS_H
#define INSIGHT_FANUMERICS_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {

/**
 Manages basic settings in faSchemes, faSolution
 */
class FaNumerics
    : public OpenFOAMCaseElement
{
public:

#include "fanumerics__FaNumerics__Parameters.h"

/*
PARAMETERSET>>> FaNumerics Parameters

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    FaNumerics ( OpenFOAMCase& c, const ParameterSet& p = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    static std::string category() { return "Numerics"; }
    virtual bool isUnique() const;
};

} // namespace insight

#endif // INSIGHT_FANUMERICS_H
