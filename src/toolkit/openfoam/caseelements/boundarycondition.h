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
            ParameterSetInput&& ip
        ),
        LIST ( c, patchName, boundaryDict, std::move(ip) )
    );
    declareStaticFunctionTable ( defaultParameters, std::unique_ptr<ParameterSet> );
    declareType ( "BoundaryCondition" );

    BoundaryCondition (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = Parameters() );

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
