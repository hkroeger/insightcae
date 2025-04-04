#ifndef INSIGHT_GGIBCBASE_H
#define INSIGHT_GGIBCBASE_H

#include "openfoam/caseelements/boundarycondition.h"

#include "ggibcbase__GGIBCBase__Parameters_headers.h"

namespace insight {


class GGIBCBase
    : public BoundaryCondition
{

public:
#include "ggibcbase__GGIBCBase__Parameters.h"
/*
PARAMETERSET>>> GGIBCBase Parameters
inherits BoundaryCondition::Parameters

shadowPatch = string "" "Name of the opposite patch"
zone = string "" "Zone name. Usually equal to patch name."
bridgeOverlap = bool true "Whether to fix small non-overlapping areas."

createGetter
<<<PARAMETERSET
*/

public:
    GGIBCBase (
        OpenFOAMCase& c, const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = Parameters() );

    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    void modifyMeshOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const override;
};


} // namespace insight

#endif // INSIGHT_GGIBCBASE_H
