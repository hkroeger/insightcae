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


#ifndef INSIGHT_PARAMETERSET_H
#define INSIGHT_PARAMETERSET_H

#include <memory>
#include <map>
#include <vector>
#include <iostream>
#include <algorithm>

#include "base/exception.h"
#include "base/parameter.h"
#include "base/parameters/arrayparameter.h"
#include "base/parameters/subsetparameter.h"
#include "base/progressdisplayer.h"


#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#include "boost/algorithm/string.hpp"

#include "rapidxml/rapidxml.hpp"


class QoccViewWidget;
class QModelTree;
class QIcon;


namespace insight {






class AnalysisParameterSet
: public ParameterSet
{
    std::string analysisTypeName_;

public:
    AnalysisParameterSet();

    AnalysisParameterSet(
        const std::string& analysisTypeName );

    const std::string& analysisTypeName() const;

    void readFromRootNode(
        const rapidxml::xml_node<>& rootNode,
        const std::string& startAtSubnode = std::string() ) override;

    typedef std::pair<std::string,std::string>
        ParameterPath_SubNodePath;

    void mergeIncompatibleParameterSet(
        const boost::filesystem::path &f,
        boost::optional<ParameterPath_SubNodePath>  subset =
            boost::optional<ParameterPath_SubNodePath>() );

    rapidxml::xml_node<>* appendToNode
        (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node
            ) const override;

    std::unique_ptr<Element> clone() const override;
};







class ParameterSet_Validator
{
public:
    typedef std::map<std::string, std::string> WarningList;
    typedef std::map<std::string, std::string> ErrorList;

protected:
    ParameterSet* ps_;


    WarningList warnings_;
    ErrorList errors_;

public:
    virtual ~ParameterSet_Validator();

    /**
     * @brief update
     * @param ps
     * checks a parameter set (needs to be customized by derivation for that)
     * Stores a copy of the parameter set and updates the warnings and errors list.
     * Needs to be called first in derived classes.
     */
    virtual void update(const ParameterSet& ps);

    virtual bool isValid() const;
    virtual const WarningList& warnings() const;
    virtual const ErrorList& errors() const;
};


typedef std::shared_ptr<ParameterSet_Validator> ParameterSet_ValidatorPtr;




}


#endif // INSIGHT_PARAMETERSET_H
