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

#include "hydrostaticpressure.h"
#include "base/analysis.h"
#include "base/cppextensions.h"
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/tools.h"
#include "base/translations.h"

#include "base/units.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/createpatch.h"

#include "boost/regex.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"

#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <map>
#include <cmath>
#include <limits>

#include "vtkSTLReader.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkCellData.h"
#include "base/warningdispatcher.h"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{


si::Pressure HydrostaticPressureComputer::calcp(si::Pressure p0, si::Acceleration g, const Liquid& l, si::Length z)
{
    return p0-l.rho*g*z;
}

si::Pressure HydrostaticPressureComputer::calcp(si::Pressure p0, si::Acceleration g, const Gas& gas, si::Length z)
{
    return p0*exp(-z*g/gas.R/si::tempDiffToZero(gas.T));
}

HydrostaticPressureComputer::HydrostaticPressureComputer(
    const arma::mat &pSurf,
    const arma::mat &eUp,
    Fluid fluid,
    si::Pressure p0Amb,
    si::Acceleration g
    )
  : pSurf(pSurf),
    eUp(eUp),
    fluid(fluid),
    p0Amb(p0Amb),
    g(g)
{}

void HydrostaticPressureComputer::operator()(
    const boost::filesystem::path &location,
    const std::string& fieldName,
    const std::vector<std::pair<std::string, std::string> > &targetEntriesPerPatch,
    bool setInternalField ) const
{
    OpenFOAMCase cm;

    std::string expr;

    std::string gh =
        str(boost::format(
             "(%g*(pos()-vector(%g,%g,%g))&vector(%g,%g,%g))")
         % toValue(g, si::meters_per_second_squared)
         % pSurf(0) % pSurf(1) % pSurf(2)
         % eUp(0) % eUp(1) % eUp(2)
         );

    if (auto *l =boost::get<Liquid>(&fluid))
    {
        if (fieldName=="p")
        {
            expr = str(
                boost::format("%g - %s*%g")
                % toValue(p0Amb, si::pascals)
                % gh
                % toValue(l->rho, si::kilogram_per_cubic_meter)
            );
        }
        else if (fieldName=="p_rgh")
        {
            expr = str(
                boost::format("%g")
                % toValue(p0Amb, si::pascals)
                );
        }
    }
    else if (auto *gas =boost::get<Gas>(&fluid))
    {
        std::string RT=str(boost::format("%g")
                             % (toValue(gas->R, si::joule/si::kilogram/si::kelvin)
                                * toValue(gas->T, si::degK) )
                             );

        if (fieldName=="p")
        {
            expr = str(
                boost::format("%g * exp(-%g/%g)")
                % toValue(p0Amb, si::pascals)
                % gh % RT
                );
        }
        else if (fieldName=="p_rgh")
        {

            expr = str(
                boost::format("%g * (%s + %s) * exp(-%s/%s) / %s")
                % toValue(p0Amb, si::pascals)
                % gh % RT % gh % RT % RT
                );
        }
    }

    insight::assertion(
        !expr.empty(), "internal error: empty expression");

    if (setInternalField)
    {
        OFDictData::dictFile sefd;
        sefd["defaultFieldValues"]=
            OFDictData::list{str(boost::format("volScalarFieldValue "+fieldName+" %g")%p0Amb)};

        OFDictData::dict p;
        p["field"]=fieldName;
        p["constants"]=OFDictData::dict();
        p["variables"]=OFDictData::list();
        p["expression"]="#{"+expr+"#}";
        sefd["expressions"]=OFDictData::list{ fieldName, p };

        sefd.write( location / "system" / "setExprFieldsDict" );

        cm.executeCommand(location, "setExprFields", {});
    }

    if (targetEntriesPerPatch.size())
    {
        OFDictData::dict pat;
        pat["field"]=fieldName;
        pat["keepPatches"]=true;

        auto addExpr = [&](const std::string& patch, const std::string& targetEntry)
        {
            OFDictData::dict exprDict;
            exprDict["patch"]=patch;
            exprDict["target"]=targetEntry;
            exprDict["expression"]="#{ "+expr+" #}";
            return exprDict;
        };

        OFDictData::dict boundaryDict;
        cm.parseBoundaryDict(location, boundaryDict);

        OFDictData::list exprs;
        for (const auto& tepp: targetEntriesPerPatch)
        {
            boost::regex re(tepp.first);
            for (const auto& p: boundaryDict)
            {
                if (boost::regex_match(p.first, re))
                {
                    exprs.push_back(addExpr(p.first, tepp.second));
                }
            }
        }
        pat["expressions"]=exprs;

        OFDictData::dictFile sebfd;
        sebfd["pattern"]=pat;

        sebfd.write( location / "system" / "setExprBoundaryFieldsDict" );

        cm.executeCommand(location, "setExprBoundaryFields", {});
    }
}

void setHydrostaticPressure(
    const OpenFOAMCase &,
    const boost::filesystem::path &location,
    const arma::mat &pSurf,
    const arma::mat &eUp,
    double rho, double p0Amb,
    const std::map<std::string, std::string> &targetEntriesPerPatch,
    const std::string& fieldName,
    bool setInternalField )
{
    OpenFOAMCase cm;

    std::string expr = str(
        boost::format("%g - (pos()-vector(%g,%g,%g))&vector(%g,%g,%g)*%g*9.81")
            % p0Amb
            % pSurf(0) % pSurf(1) % pSurf(2)
            % eUp(0) % eUp(1) % eUp(2)
            % rho
        );

    if (setInternalField)
    {
        OFDictData::dictFile sefd;
        sefd["defaultFieldValues"]=
            OFDictData::list{str(boost::format("volScalarFieldValue "+fieldName+" %g")%p0Amb)};

        OFDictData::dict p;
        p["field"]=fieldName;
        p["constants"]=OFDictData::dict();
        p["variables"]=OFDictData::list();
        p["expression"]="#{"+expr+"#}";
        sefd["expressions"]=OFDictData::list{ fieldName, p };

        sefd.write( location / "system" / "setExprFieldsDict" );

        cm.executeCommand(location, "setExprFields", {});
    }

    if (targetEntriesPerPatch.size())
    {
        OFDictData::dict pat;
        pat["field"]=fieldName;
        pat["keepPatches"]=true;

        auto addExpr = [&](const std::string& patch, const std::string& targetEntry)
        {
            OFDictData::dict exprDict;
            exprDict["patch"]=patch;
            exprDict["target"]=targetEntry;
            exprDict["expression"]="#{ "+expr+" #}";
            return exprDict;
        };

        OFDictData::dict boundaryDict;
        cm.parseBoundaryDict(location, boundaryDict);

        OFDictData::list exprs;
        for (const auto& tepp: targetEntriesPerPatch)
        {
            boost::regex re(tepp.first);
            for (const auto& p: boundaryDict)
            {
                if (boost::regex_match(p.first, re))
                {
                    exprs.push_back(addExpr(p.first, tepp.second));
                }
            }
        }
        pat["expressions"]=exprs;

        OFDictData::dictFile sebfd;
        sebfd["pattern"]=pat;

        sebfd.write( location / "system" / "setExprBoundaryFieldsDict" );

        cm.executeCommand(location, "setExprBoundaryFields", {});
    }
}


}
