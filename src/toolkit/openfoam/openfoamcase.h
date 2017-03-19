/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#ifndef INSIGHT_OPENFOAMCASE_H
#define INSIGHT_OPENFOAMCASE_H

#include "base/case.h"
#include "openfoam/openfoamdict.h"
#include "base/softwareenvironment.h"
#include "boost/ptr_container/ptr_map.hpp"


namespace insight {

    
    
    
class ProgressDisplayer;




enum FieldType
{
  scalarField,
  vectorField,
  symmTensorField
};




enum FieldGeoType
{
  volField,
  pointField,
  tetField
};




extern const OFDictData::dimensionSet dimPressure;
extern const OFDictData::dimensionSet dimKinPressure;
extern const OFDictData::dimensionSet dimKinEnergy;
extern const OFDictData::dimensionSet dimVelocity;
extern const OFDictData::dimensionSet dimLength;
extern const OFDictData::dimensionSet dimDensity;
extern const OFDictData::dimensionSet dimless;
extern const OFDictData::dimensionSet dimKinViscosity;
extern const OFDictData::dimensionSet dimDynViscosity;
extern const OFDictData::dimensionSet dimTemperature;
extern const OFDictData::dimensionSet dimCurrent;




typedef std::vector<double> FieldValue;
typedef boost::fusion::tuple<FieldType, OFDictData::dimensionSet, FieldValue, FieldGeoType > FieldInfo;
typedef std::map<std::string, FieldInfo> FieldList;




struct OFdicts
: public boost::ptr_map<std::string, OFDictData::dictFile> 
{
  OFDictData::dictFile& addDictionaryIfNonexistent(const std::string& key);
  OFDictData::dictFile& addFieldIfNonexistent(const std::string& key, const FieldInfo& fi);
  OFDictData::dictFile& lookupDict(const std::string& key);
};




class OpenFOAMCase;




class OFEnvironment
    : public SoftwareEnvironment
{
protected:
    int version_;
    boost::filesystem::path bashrc_;

public:
    OFEnvironment ( int version, const boost::filesystem::path& bashrc );

    virtual int version() const;
    virtual const boost::filesystem::path& bashrc() const;
    //virtual int executeCommand(const std::vector<std::string>& args) const;
};




class OFEs
    : public boost::ptr_map<std::string, OFEnvironment>
{
public:
    static OFEs list;

    static std::vector<std::string> all();
    static const OFEnvironment& get ( const std::string& name );
    
    /**
     * inspects WM_PROJECT_DIR env variable and returns name of currently loaded OFE. Empty string, if none is set
     */
    static std::string detectCurrentOFE();

    OFEs();
    ~OFEs();
};



#define addToOpenFOAMCaseElementFactoryTable(DerivedClass) \
 addToFactoryTable(OpenFOAMCaseElement, DerivedClass); \
 addToStaticFunctionTable(OpenFOAMCaseElement, DerivedClass, defaultParameters); \
 addToStaticFunctionTable(OpenFOAMCaseElement, DerivedClass, category)
 

class OpenFOAMCaseElement
    : public CaseElement
{

public:
    declareFactoryTable ( OpenFOAMCaseElement, LIST ( OpenFOAMCase& c, const ParameterSet& ps ), LIST ( c, ps ) );
    declareStaticFunctionTable ( defaultParameters, ParameterSet );
    declareStaticFunctionTable ( category, std::string );
    declareType ( "OpenFOAMCaseElement" );

    OpenFOAMCaseElement ( OpenFOAMCase& c, const std::string& name );

    // defined below declaration of OpenFOAMCase
    inline const OpenFOAMCase& OFcase() const;
    inline OpenFOAMCase& OFcase();

    int OFversion() const;
    virtual void modifyMeshOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const;
    virtual void modifyCaseOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const;
    virtual void addFields( OpenFOAMCase& c ) const;
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const =0;

    virtual bool providesBCsForPatch ( const std::string& patchName ) const;
    
    static std::string category() { return "Uncategorized"; }
};


typedef boost::shared_ptr<OpenFOAMCaseElement> OpenFOAMCaseElementPtr;




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
    
    BoundaryCondition ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict );
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const =0;
    virtual void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const;
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

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

    virtual bool providesBCsForPatch ( const std::string& patchName ) const;
};




class SolverOutputAnalyzer
{

protected:
    ProgressDisplayer& pdisp_;

    double curTime_;
    std::map<std::string, double> curProgVars_;

    /**
     * name of currently tracked force output,
     * emtpy, if no force output is currently expected
     */
    std::string curforcename_;
    int curforcesection_;
    arma::mat curforcevalue_;

//   std::map<std::string, std::vector<arma::mat> > trackedForces_;

public:
    SolverOutputAnalyzer ( ProgressDisplayer& pdisp );

    void update ( const std::string& line );

    bool stopRun() const;
};




class OpenFOAMCase
    : public Case
{
public:
    typedef enum {
        cellVolumeWeightMapMethod,
        directMapMethod
    } MapMethod;

protected:
    const OFEnvironment& env_;
    FieldList fields_;
    bool fieldListCompleted_ = false;
    MapMethod requiredMapMethod_;
    
    void createFieldListIfRequired() const;

public:
    OpenFOAMCase ( const OFEnvironment& env );
    OpenFOAMCase ( const OpenFOAMCase& other );
    virtual ~OpenFOAMCase();

    inline void setRequiredMapMethod ( const MapMethod mm )
    {
        requiredMapMethod_=mm;
    }
    inline MapMethod requiredMapMethod() const
    {
        return requiredMapMethod_;
    }

    void addField ( const std::string& name, const FieldInfo& field );

    void parseBoundaryDict ( const boost::filesystem::path& location, OFDictData::dict& boundaryDict ) const;

    std::set<std::string> getUnhandledPatches ( OFDictData::dict& boundaryDict ) const;

    template<class BC>
    void addRemainingBCs ( OFDictData::dict& boundaryDict, const typename BC::Parameters& params )
    {
        typedef std::set<std::string> StringSet;
        StringSet unhandledPatches = getUnhandledPatches ( boundaryDict );

        for ( StringSet::const_iterator i=unhandledPatches.begin(); i!=unhandledPatches.end(); i++ ) {
            insert ( new BC ( *this, *i, boundaryDict, params ) );
        }
    }

    void addRemainingBCs ( const std::string& bc_type, OFDictData::dict& boundaryDict, const ParameterSet& ps );
    
    inline const OFEnvironment& ofe() const
    {
        return env_;
    }
    inline int OFversion() const
    {
        return env_.version();
    }

    bool isCompressible() const;

    boost::shared_ptr<OFdicts> createDictionaries() const;
    void modifyMeshOnDisk ( const boost::filesystem::path& location ) const;
    void modifyCaseOnDisk ( const boost::filesystem::path& location ) const;

    virtual void createOnDisk ( const boost::filesystem::path& location, boost::shared_ptr<OFdicts> dictionaries );
    virtual void createOnDisk ( const boost::filesystem::path& location );

    virtual bool meshPresentOnDisk ( const boost::filesystem::path& location ) const;
    virtual bool outputTimesPresentOnDisk ( const boost::filesystem::path& location, bool checkpar=false ) const;
    virtual void removeProcessorDirectories ( const boost::filesystem::path& location ) const;

    std::string cmdString
    (
        const boost::filesystem::path& location,
        const std::string& cmd,
        std::vector<std::string> argv
    )
    const;

    void executeCommand
    (
        const boost::filesystem::path& location,
        const std::string& cmd,
        std::vector<std::string> argv = std::vector<std::string>(),
        std::vector<std::string>* output = NULL,
        int np=0,
        std::string *ovr_machine=NULL
    ) const;

    void runSolver
    (
        const boost::filesystem::path& location,
        SolverOutputAnalyzer& analyzer,
        std::string solverName,
        bool *stopFlag = NULL,
        int np=0,
        const std::vector<std::string>& addopts = std::vector<std::string>()
    ) const;

    template<class stream>
    void forkCommand
    (
        stream& p_in,
        const boost::filesystem::path& location,
        const std::string& cmd,
        std::vector<std::string> argv = std::vector<std::string>(),
        std::string *ovr_machine=NULL
    ) const
    {
        env_.forkCommand ( p_in, cmdString ( location, cmd, argv ), std::vector<std::string>(), ovr_machine );
    }

    inline const FieldList& fields() const
    {
        createFieldListIfRequired();
        return fields_;
    }
    inline FieldList& fields()
    {
        createFieldListIfRequired();
        return fields_;
    }

    std::vector<std::string> fieldNames() const;

    inline FieldInfo& field ( const std::string& fname )
    {
        createFieldListIfRequired();
        return fields_.find ( fname )->second;
    }

    template<class T>
    const T& findUniqueElement() const
    {
        const T* the_e = NULL;

        bool found=false;
        for ( boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
                i!=elements_.end(); i++ ) {
            const T *e= dynamic_cast<const T*> ( & ( *i ) );
            if ( e ) {
                if ( found ) {
                    throw insight::Exception ( "OpenFOAMCase::findUniqueElement(): Multiple elements of requested type "+T::typeName+" in queried OpenFOAM case!" );
                }
                the_e=e;
                found=true;
            }
        }
        if ( !found ) {
            throw insight::Exception ( "OpenFOAMCase::findUniqueElement(): No element of requested type "+T::typeName+" in queried OpenFOAM case found !" );
        }

        return *the_e;
    }

    template<class T>
    T& getUniqueElement()
    {
        return const_cast<T&> ( findUniqueElement<T>() );
    }

    void setFromXML(const std::string& xml, const boost::filesystem::path& file, bool skipOFE=true, bool skipBCs=false, const boost::filesystem::path& casepath="");
    void loadFromFile(const boost::filesystem::path& file, bool skipOFE=true, bool skipBCs=false, const boost::filesystem::path& casepath="");
};




const OpenFOAMCase& OpenFOAMCaseElement::OFcase() const { return *static_cast<OpenFOAMCase*>(&case_); }
OpenFOAMCase& OpenFOAMCaseElement::OFcase() { return *static_cast<OpenFOAMCase*>(&case_); }




}

#endif // INSIGHT_OPENFOAMCASE_H
