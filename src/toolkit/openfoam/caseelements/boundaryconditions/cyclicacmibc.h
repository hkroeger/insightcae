#ifndef INSIGHT_CYCLICACMI_H
#define INSIGHT_CYCLICACMI_H

#include "openfoam/caseelements/boundaryconditions/ggibcbase.h"

namespace insight {

class CyclicACMIBC
        : public GGIBCBase
{
public:
#include "cyclicacmibc__CyclicACMIBC__Parameters.h"
/*
PARAMETERSET>>> CyclicACMIBC Parameters
inherits GGIBCBase::Parameters

//modifyMesh = bool false "Create auxiliary patch and baffles."

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "CyclicACMIBC" );
    CyclicACMIBC(
        OpenFOAMCase& c, const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = Parameters() );

    void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const override;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;
    void modifyMeshOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const override;

#ifndef SWIG
    static void modifyMesh (
            const OpenFOAMCase& cm,
            const boost::filesystem::path& location,
            const std::vector<std::pair<std::string, CyclicACMIBC::Parameters> >& patchNames_Parameters );
#endif

    static void modifyMesh (
        const OpenFOAMCase& cm,
        const boost::filesystem::path& location,
        const std::map<std::string, std::string>& patchNames_shadowPatchNames );

    static std::string uncoupledPatchName(const std::string& patchName, const CyclicACMIBC::Parameters& p);
    inline std::string uncoupledPatchName() const { return uncoupledPatchName(patchName(), p()); }

};

} // namespace insight

#endif // INSIGHT_CYCLICACMI_H
