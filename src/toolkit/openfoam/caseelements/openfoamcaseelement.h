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
