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


#ifndef INSIGHT_VARIABLENAMES_H
#define INSIGHT_VARIABLENAMES_H

#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include <boost/algorithm/string.hpp>

#include "base/exception.h"

namespace insight {

template<class Roles>
class VariableNames
    : public std::map<Roles, std::string>
{
public:
    VariableNames(
        std::initializer_list<
            typename std::map< Roles, std::string>::value_type
            > ini)
        : std::map<Roles, std::string>(ini)
    {}

    Roles variable(const std::string& orgVarName) const
    {
        auto varName=boost::to_upper_copy(orgVarName);
        auto i = std::find_if(
            this->begin(), this->end(),
            [&](const typename std::map<Roles, std::string>::value_type& entry)
            {
                return entry.second==varName;
            }
            );

        if (i==this->end())
        {
            std::vector<std::string> sel;
            std::transform(
                this->begin(), this->end(),
                std::back_inserter(sel),
                [](const typename std::map<Roles, std::string>::value_type& v)
                { return v.second; }
                );
            throw insight::Exception(
                "unknown variable name: %s. Recognized names are: %s",
                varName.c_str(),
                boost::join(sel, ", ").c_str() );
        }

        return i->first;
    }
};

}

#endif // INSIGHT_VARIABLENAMES_H
