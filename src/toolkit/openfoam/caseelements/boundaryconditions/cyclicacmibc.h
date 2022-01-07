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

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "CyclicACMIBC" );
    CyclicACMIBC( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
                  const ParameterSet&ps = Parameters::makeDefault() );

    void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const override;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;
    void modifyMeshOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const override;

    static void modifyMesh (
            const OpenFOAMCase& cm,
            const boost::filesystem::path& location,
            const std::vector<std::pair<const std::string&, const Parameters&> >& patchNames_Parameters );

    static std::string uncoupledPatchName(const std::string& patchName, const Parameters& p);
    inline std::string uncoupledPatchName() const { return uncoupledPatchName(patchName(), p_); }

};

} // namespace insight

#endif // INSIGHT_CYCLICACMI_H
