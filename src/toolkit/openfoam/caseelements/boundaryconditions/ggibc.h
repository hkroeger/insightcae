#ifndef INSIGHT_GGIBC_H
#define INSIGHT_GGIBC_H


#include "openfoam/caseelements/boundaryconditions/ggibcbase.h"

namespace insight {


class GGIBC
    : public GGIBCBase
{

public:
#include "ggibc__GGIBC__Parameters.h"
/*
PARAMETERSET>>> GGIBC Parameters
inherits GGIBCBase::Parameters

separationOffset = vector (0 0 0) "Translational transformation from this patch to the opposite one."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "GGIBC" );
    GGIBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
            const ParameterSet&ps = Parameters::makeDefault() );
    void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const override;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;
};


} // namespace insight

#endif // INSIGHT_GGIBC_H
