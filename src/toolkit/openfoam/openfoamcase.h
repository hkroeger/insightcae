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

#include "openfoam/ofenvironment.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/ofdicts.h"


namespace insight {

    
    
class ProgressDisplayer;
class SolverOutputAnalyzer;



class OpenFOAMCase
    : public Case
{
public:
    typedef enum {
        cellVolumeWeightMapMethod,
        directMapMethod
    } MapMethod;

protected:
    OFEnvironment env_;
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

    boost::filesystem::path boundaryDictPath(const boost::filesystem::path& location) const;
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

    void modifyFilesOnDiskBeforeDictCreation ( const boost::filesystem::path& location ) const;
    std::shared_ptr<OFdicts> createDictionaries() const;
    virtual void modifyMeshOnDisk ( const boost::filesystem::path& location ) const;
    virtual void modifyCaseOnDisk ( const boost::filesystem::path& location ) const;

    virtual void createOnDisk 
    ( 
        const boost::filesystem::path& location, 
        std::shared_ptr<OFdicts> dictionaries, 
        const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles = std::shared_ptr<std::vector<boost::filesystem::path> >()
    );
    virtual void createOnDisk 
    ( 
        const boost::filesystem::path& location, 
        const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles = std::shared_ptr<std::vector<boost::filesystem::path> >()
    );

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
        int np=0,
        const std::vector<std::string>& addopts = std::vector<std::string>()
    ) const;

    SoftwareEnvironment::JobPtr forkCommand
    (
        const boost::filesystem::path& location,
        const std::string& cmd,
        std::vector<std::string> argv = std::vector<std::string>(),
        std::string *ovr_machine = nullptr
    ) const;

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

    bool hasField(const std::string& fname ) const;

    bool hasPrghPressureField() const;

    inline FieldInfo& field ( const std::string& fname )
    {
        createFieldListIfRequired();
        return fields_.find ( fname )->second;
    }


    void setFromXML(const std::string& xml, const boost::filesystem::path& file, bool skipOFE=true, bool skipBCs=false, const boost::filesystem::path& casepath="");
    void loadFromFile(const boost::filesystem::path& file, bool skipOFE=true, bool skipBCs=false, const boost::filesystem::path& casepath="");
    

    virtual bool hasCyclicBC() const;

    OFDictData::dict diagonalSolverSetup() const;
    OFDictData::dict stdAsymmSolverSetup(double tol=1e-7, double reltol=0.0, int minIter=0) const;
    OFDictData::dict stdSymmSolverSetup(double tol=1e-7, double reltol=0.0, int maxIter=1000) const;
    OFDictData::dict smoothSolverSetup(double tol=1e-7, double reltol=0.0, int minIter=0) const;
    OFDictData::dict GAMGSolverSetup(double tol=1e-7, double reltol=0.0) const;
    OFDictData::dict GAMGPCGSolverSetup(double tol=1e-7, double reltol=0.0) const;

};



#ifdef SWIG
%template(getOpenFOAMCaseElement) insight::OpenFOAMCase::get<const OpenFOAMCaseElement>;
#endif


}

#endif // INSIGHT_OPENFOAMCASE_H
