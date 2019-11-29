#ifndef INSIGHT_BOUNDARYCONDITIONCASEELEMENT_H
#define INSIGHT_BOUNDARYCONDITIONCASEELEMENT_H

#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/ofdicts.h"

namespace insight {

/*
 * Manages the configuration of a single patch, i.e. one BoundaryCondition-object
 * needs to know proper BC's for all fields on the given patch
 */
class BoundaryCondition
    : public OpenFOAMCaseElement
{
protected:
    std::string patchName_;
    std::string BCtype_;
    int nFaces_;
    int startFace_;

public:
    declareFactoryTable
    (
        BoundaryCondition,
        LIST
        (
            OpenFOAMCase& c,
            const std::string& patchName,
            const OFDictData::dict& boundaryDict,
            const ParameterSet& ps
        ),
        LIST ( c, patchName, boundaryDict, ps )
    );
    declareStaticFunctionTable ( defaultParameters, ParameterSet );
    declareType ( "BoundaryCondition" );

    BoundaryCondition ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& ps );
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const =0;
    virtual void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const;
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static void insertIntoBoundaryDict
    (
        OFdicts& dictionaries,
        const std::string& patchName,
        const OFDictData::dict& bndsubd
    );

    inline const std::string patchName() const
    {
        return patchName_;
    }
    inline const std::string BCtype() const
    {
        return BCtype_;
    }

    bool providesBCsForPatch ( const std::string& patchName ) const override;

    static bool isPrghPressureField(const FieldList::value_type& fieldinfo);

};




} // namespace insight

#endif // INSIGHT_BOUNDARYCONDITIONCASEELEMENT_H
