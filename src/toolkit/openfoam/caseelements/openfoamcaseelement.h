#ifndef INSIGHT_OPENFOAMCASEELEMENT_H
#define INSIGHT_OPENFOAMCASEELEMENT_H


#include "base/caseelement.h"


namespace insight {


class OpenFOAMCase;
class OFdicts;
class ParameterSetVisualizer;

namespace OFDictData { class dict; }

#define addToOpenFOAMCaseElementFactoryTable(DerivedClass) \
 addToFactoryTable(OpenFOAMCaseElement, DerivedClass); \
 addToCaseElementFactoryTable(DerivedClass); \
 addToStaticFunctionTable(OpenFOAMCaseElement, DerivedClass, defaultParameters); \
 addToStaticFunctionTable(OpenFOAMCaseElement, DerivedClass, category);


class OpenFOAMCaseElement
    : public CaseElement
{

public:
    declareFactoryTable ( OpenFOAMCaseElement, LIST ( OpenFOAMCase& c, const ParameterSet& ps ), LIST ( c, ps ) );
    declareStaticFunctionTable ( defaultParameters, ParameterSet );
    declareStaticFunctionTable ( category, std::string );
    declareStaticFunctionTable (validator, ParameterSet_ValidatorPtr);
    declareStaticFunctionTable (visualizer, std::shared_ptr<ParameterSetVisualizer>);
    declareType ( "OpenFOAMCaseElement" );

    OpenFOAMCaseElement ( OpenFOAMCase& c, const std::string& name, const ParameterSet& ps );

    // defined below declaration of OpenFOAMCase
    const OpenFOAMCase& OFcase() const;
    OpenFOAMCase& OFcase();

    int OFversion() const;
    virtual void modifyFilesOnDiskBeforeDictCreation ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const;
    virtual void modifyMeshOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const;
    virtual void modifyCaseOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const;
    virtual void addFields( OpenFOAMCase& c ) const;
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const =0;

    virtual bool providesBCsForPatch ( const std::string& patchName ) const;

    static std::string category();
    static ParameterSet_ValidatorPtr validator();
    static std::shared_ptr<ParameterSetVisualizer> visualizer();
    static bool isInConflict(const CaseElement& other);


};


typedef std::shared_ptr<OpenFOAMCaseElement> OpenFOAMCaseElementPtr;



} // namespace insight

#endif // INSIGHT_OPENFOAMCASEELEMENT_H
