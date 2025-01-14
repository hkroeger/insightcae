#ifndef INSIGHT_CYCLICPAIRBC_H
#define INSIGHT_CYCLICPAIRBC_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {

namespace OFDictData { class dict; }


class CyclicPairBC
    : public OpenFOAMCaseElement
{

protected:
    std::string patchName_;
    int nFaces_, nFaces1_;
    int startFace_, startFace1_;

public:
    declareType ( "CyclicPairBC" );
    CyclicPairBC (
        OpenFOAMCase& c, const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;

    bool providesBCsForPatch ( const std::string& patchName ) const override;

};


} // namespace insight

#endif // INSIGHT_CYCLICPAIRBC_H
