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

#ifndef BASICNUMERICSCASEELEMENTS_H
#define BASICNUMERICSCASEELEMENTS_H


#include "base/linearalgebra.h"
#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"
#include "base/boost_include.h"
#include "openfoam/caseelements/numerics/pimplesettings.h"
#include "openfoam/caseelements/basiccaseelements.h"

#include <map>
#include "boost/utility.hpp"
#include "boost/variant.hpp"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight
{

class OpenFOAMCase;
class OFdicts;



/**
 Manages basic settings in controlDict, fvSchemes, fvSolution, list of fields
 */


class FVNumerics
    : public decomposeParDict
{
public:
    declareType ( "FVNumerics" );

public:
#include "basicnumericscaseelements__FVNumerics__Parameters.h"

/*
PARAMETERSET>>> FVNumerics Parameters

inherits decomposeParDict::Parameters

writeControl = selection (
    adjustableRunTime
    clockTime
    cpuTime
    runTime
    timeStep
) timeStep "Type of write control"

writeInterval = double 100.0 "Write interval"

writeFormat = selection ( ascii binary ) ascii "Write format"

purgeWrite = int 10 "Purge write interval, set to 0 to disable"

deltaT = double 1.0 "Time step size. If the time step is selected to be adjustable, this is the initial time step size."

endTime = double 1000.0 "Maximum end time of simulation"


mapFieldsConfig = selectablesubset {{
 none set {}
 map set {
  patchMap = array [ set {
     targetPatch = string "lid" "Name of patch in target mesh"
     sourcePatch = string "movingWall" "Name of patch in source mesh"
  }] *0 "Pairs of patches for mapping"

  cuttingPatches = array [
    string "fixedWalls" "Name of patch in target mesh"
  ] *0 "Patches whose values shall be interpolated from source interior"
 }
}} map "Mapfield configuration"

<<<PARAMETERSET
*/

protected:
    Parameters p_;
    std::string pName_;

public:
    FVNumerics ( OpenFOAMCase& c, const ParameterSet& ps, const std::string& pName );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    virtual bool isCompressible() const =0;
    virtual bool isLES() const;
    virtual bool isGAMGOk() const;

    void setApplicationName(OFdicts& dictionaries, const std::string& appname) const;
    void setRelaxationFactors
    (
        OFdicts& dictionaries,
        const std::map<std::string, double>& eqnRelax,
        const std::map<std::string, double>& fieldRelax
    ) const;

    std::string lqGradSchemeIfPossible() const;
    void insertStandardGradientConfig(OFdicts& dictionaries) const;

    std::string gradNameOrScheme(OFdicts& dictionaries, const std::string& key) const;

    static std::string category() { return "Numerics"; }

    virtual bool isUnique() const;
};







class potentialFoamNumerics
    : public FVNumerics
{

public:
#include "basicnumericscaseelements__potentialFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> potentialFoamNumerics Parameters
inherits FVNumerics::Parameters

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "potentialFoamNumerics" );
    potentialFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
    static ParameterSet defaultParameters();
};






class laplacianFoamNumerics
    : public FVNumerics
{

public:
#include "basicnumericscaseelements__laplacianFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> laplacianFoamNumerics Parameters
inherits FVNumerics::Parameters

Tinternal = double 300.0 "[K] Initial temperature value in internal field"
DT = double 1e-6 "[m^2/s] Constant diffivity"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "laplacianFoamNumerics" );
    laplacianFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
    static ParameterSet defaultParameters();
};




/**
 Manages basic settings in faSchemes, faSolution
 */
class FaNumerics
    : public OpenFOAMCaseElement
{
public:

#include "basicnumericscaseelements__FaNumerics__Parameters.h"

/*
PARAMETERSET>>> FaNumerics Parameters

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    FaNumerics ( OpenFOAMCase& c, const ParameterSet& p = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    static std::string category() { return "Numerics"; }
    virtual bool isUnique() const;
};




/**
 Manages basic settings in tetFemSolution
 */
class tetFemNumerics
    : public OpenFOAMCaseElement
{

public:
    tetFemNumerics ( OpenFOAMCase& c );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    static std::string category() { return "Numerics"; }
    virtual bool isUnique() const;
};




class MeshingNumerics
    : public decomposeParDict
{
public:
#include "basicnumericscaseelements__MeshingNumerics__Parameters.h"

/*
PARAMETERSET>>> MeshingNumerics Parameters

inherits decomposeParDict::Parameters

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "MeshingNumerics" );

    MeshingNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static ParameterSet defaultParameters();
};




}


#endif // BASICNUMERICSCASEELEMENTS_H
