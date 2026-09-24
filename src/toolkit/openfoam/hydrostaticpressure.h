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

#ifndef INSIGHT_HYDROSTATICPRESSURE_H
#define INSIGHT_HYDROSTATICPRESSURE_H

#include <map>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/variant.hpp>

#include "base/linearalgebra.h"
#include "base/units.h"

namespace insight
{

class OpenFOAMCase;

class HydrostaticPressureComputer
{
public:
    struct Gas
    {
        si::SpecificHeatCapacity R;
        si::Temperature T;
    };

    struct Liquid
    {
        si::Density rho;
    };

    typedef boost::variant<Gas,Liquid> Fluid;

    static si::Pressure calcp(
        si::Pressure p0, si::Acceleration g,
        const Liquid& l, si::Length z);

    static si::Pressure calcp(
        si::Pressure p0, si::Acceleration g,
        const Gas& gas, si::Length z);

private:
    si::Acceleration g;
    arma::mat pSurf;
    arma::mat eUp;
    Fluid fluid;
    si::Pressure p0Amb;

public:
    HydrostaticPressureComputer(
        const arma::mat &pSurf,
        const arma::mat &eUp,
        Fluid fluid,
        si::Pressure p0Amb,
        si::Acceleration g = 9.81*si::meters_per_second_squared
        );

    void operator()(
        const boost::filesystem::path &location,
        const std::string& fieldName,
        const std::vector<std::pair<std::string, std::string> > &targetEntriesPerPatch,
        bool setInternalField = true ) const;
};


void setHydrostaticPressure(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseDir,
    const arma::mat& pSurf,
    const arma::mat& eUp,
    double rho, double p0Amb,
    const std::map<std::string, std::string> &targetEntriesPerPatch = {},
    const std::string& fieldName = "p",
    bool setInternalField = true
    );

}

#endif // INSIGHT_HYDROSTATICPRESSURE_H
