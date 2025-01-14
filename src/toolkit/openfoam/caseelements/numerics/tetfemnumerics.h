#ifndef INSIGHT_TETFEMNUMERICS_H
#define INSIGHT_TETFEMNUMERICS_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {

/**
 Manages basic settings in tetFemSolution
 */
class tetFemNumerics
    : public OpenFOAMCaseElement
{

public:
    tetFemNumerics ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    static std::string category() { return "Numerics"; }
    virtual bool isUnique() const;
};

} // namespace insight

#endif // INSIGHT_TETFEMNUMERICS_H
