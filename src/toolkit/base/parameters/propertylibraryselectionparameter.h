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

#ifndef INSIGHT_PROPERTYLIBRARYSELECTIONPARAMETER_H
#define INSIGHT_PROPERTYLIBRARYSELECTIONPARAMETER_H

#include "base/parameters/simpleparameter.h"
#include "base/propertylibrary.h"

namespace insight {

class PropertyLibrarySelectionParameter
        : public StringParameter
{

protected:
    const PropertyLibraryBase* propertyLibrary_;

public:
    declareType ( "librarySelection" );

    PropertyLibrarySelectionParameter (
            const std::string& description,
            bool isHidden=false,
            bool isExpert=false,
            bool isNecessary=false, int order=0 );

    PropertyLibrarySelectionParameter (
        const PropertyLibraryBase& lib,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    PropertyLibrarySelectionParameter (
            const std::string& value,
            const PropertyLibraryBase& lib,
            const std::string& description,
            bool isHidden=false,
            bool isExpert=false,
            bool isNecessary=false,
            int order=0 );

    bool isDifferent(const Parameter& p) const override;

    std::vector<std::string> items() const;
    bool contains(const std::string& value) const;

    void setSelection ( const std::string& sel );
    const std::string& selection() const;


    void readFromNode(
        const std::string& name,
        rapidxml::xml_node<>& node,
        boost::filesystem::path
    ) override;

    Parameter* clone() const override;
    void copyFrom(const Parameter& p) override;
    void operator=(const PropertyLibrarySelectionParameter& p);
    int nChildren() const override;
};

} // namespace insight

#endif // INSIGHT_PROPERTYLIBRARYSELECTIONPARAMETER_H
