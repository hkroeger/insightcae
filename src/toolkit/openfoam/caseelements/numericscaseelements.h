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

#ifndef INSIGHT_NUMERICSCASEELEMENTS_H
#define INSIGHT_NUMERICSCASEELEMENTS_H

#include "base/linearalgebra.h"
#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"
#include "base/boost_include.h"
#include "openfoam/caseelements/incompressiblenumericscaseelements.h"
#include "openfoam/caseelements/compressiblenumericscaseelements.h"

#include <map>
#include "boost/utility.hpp"
#include "boost/variant.hpp"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight 
{







class potentialFreeSurfaceFoamNumerics
    : public FVNumerics
{

public:
#include "numericscaseelements__potentialFreeSurfaceFoamNumerics__Parameters.h"

//  nCorrectors = int 2 "Number of correctors"
//  nOuterCorrectors = int 1 "Number of outer correctors"
//  nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"
//  maxCo = double 0.45 "Maximum courant number"
//  maxDeltaT = double 1.0 "Maximum time step size"
/*
PARAMETERSET>>> potentialFreeSurfaceFoamNumerics Parameters
inherits FVNumerics::Parameters

time_integration = includedset "insight::PIMPLESettings::Parameters" "Settings for time integration"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "potentialFreeSurfaceFoamNumerics" );
    potentialFreeSurfaceFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class simpleDyMFoamNumerics
    : public simpleFoamNumerics
{

public:
#include "numericscaseelements__simpleDyMFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> simpleDyMFoamNumerics Parameters
inherits simpleFoamNumerics::Parameters

FEMinterval = int 10 "Interval between successive FEM updates"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "simpleDyMFoamNumerics" );
    simpleDyMFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class pimpleDyMFoamNumerics
    : public pimpleFoamNumerics
{

public:
#include "numericscaseelements__pimpleDyMFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> pimpleDyMFoamNumerics Parameters
inherits pimpleFoamNumerics::Parameters

FEMinterval = int 10 "Interval between successive FEM updates"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "pimpleDyMFoamNumerics" );
    pimpleDyMFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class cavitatingFoamNumerics
    : public FVNumerics
{

public:
#include "numericscaseelements__cavitatingFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> cavitatingFoamNumerics Parameters
inherits FVNumerics::Parameters

solverName = string "cavitatingFoam" "Name of the solver"
pamb = double 1e5 "Ambient pressure value"
rhoamb = double 1 "Ambient density"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "cavitatingFoamNumerics" );
    cavitatingFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class interFoamNumerics
    : public FVNumerics
{

public:
#include "numericscaseelements__interFoamNumerics__Parameters.h"

//  implicitPressureCorrection = bool false "Whether to switch to implicit pressure correction"
//  nOuterCorrectors = int 50 "Number of outer correctors"
//  maxCo = double 5 "Maximum courant number"
//  maxAlphaCo = double 3 "Maximum courant number at interface"
/*
PARAMETERSET>>> interFoamNumerics Parameters
inherits FVNumerics::Parameters

alphainternal = double 0.0 "Internal phase fraction field value"
pinternal = double 0.0 "Internal pressure field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

time_integration = includedset "insight::MultiphasePIMPLESettings::Parameters" "Settings for time integration"


alphaSubCycles = int 1 "Number of alpha integration subcycles"

cAlpha = double 0.25 "[-] Interface compression coefficient"
icAlpha = double 0 "[-] Isotropic interface compression coefficient"

snGradLowQualityLimiterReduction = double 0.66 "Reduction of limiter coefficient on low quality faces"

<<<PARAMETERSET
*/

protected:
    Parameters p_;
    std::string pname_;
    std::string alphaname_;
    
    void init();

public:
    declareType ( "interFoamNumerics" );
    interFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    inline const std::string& pressureFieldName() const
    {
        return pname_;
    }
    inline const std::string& alphaFieldName() const
    {
        return alphaname_;
    }
    static ParameterSet defaultParameters();
};




OFDictData::dict stdMULESSolverSetup(double cAlpha, double icAlpha, double tol=1e-8, double reltol=0.0, bool LTS=false);




class LTSInterFoamNumerics
    : public interFoamNumerics
{
public:
    declareType ( "LTSInterFoamNumerics" );
    LTSInterFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class interPhaseChangeFoamNumerics
    : public interFoamNumerics
{

public:
#include "numericscaseelements__interPhaseChangeFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> interPhaseChangeFoamNumerics Parameters
inherits interFoamNumerics::Parameters

solverName = string "interPhaseChangeFoam" "Name of the solver to use"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "interPhaseChangeFoamNumerics" );
    interPhaseChangeFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class reactingFoamNumerics
    : public FVNumerics
{

public:
#include "numericscaseelements__reactingFoamNumerics__Parameters.h"

//  nCorrectors = int 2 "Number of correctors"
//  nOuterCorrectors = int 1 "Number of outer correctors"
//  nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"
//  maxCo = double 0.45 "Maximum courant number"
//  maxDeltaT = double 1.0 "Maximum time step size"
/*
PARAMETERSET>>> reactingFoamNumerics Parameters
inherits FVNumerics::Parameters


time_integration = includedset "insight::CompressiblePIMPLESettings::Parameters" "Settings for time integration"

forceLES = bool false "Whether to enforce LES numerics"

<<<PARAMETERSET
*/

protected:
    Parameters p_;
    
    void init();

public:
    declareType ( "reactingFoamNumerics" );
    reactingFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class reactingParcelFoamNumerics
    : public reactingFoamNumerics
{
public:
    declareType ( "reactingParcelFoamNumerics" );
    reactingParcelFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};


class buoyantSimpleFoamNumerics
    : public FVNumerics
{

public:
#include "numericscaseelements__buoyantSimpleFoamNumerics__Parameters.h"

//  time_integration = includedset "insight::PIMPLESettings::Parameters" "Settings for time integration"
/*
PARAMETERSET>>> buoyantSimpleFoamNumerics Parameters
inherits FVNumerics::Parameters

checkResiduals = bool false "Enable solver stop on residual goal"
nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"

Tinternal = double 300 "initial temperature in internal field"
pinternal = double 1e5 "initial pressure in internal field"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

    void init();

public:
    declareType ( "buoyantSimpleFoamNumerics" );
    buoyantSimpleFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class buoyantPimpleFoamNumerics
    : public FVNumerics
{

public:
#include "numericscaseelements__buoyantPimpleFoamNumerics__Parameters.h"

//  nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"
//  nCorrectors = int 1 "Number of correctors"
//  nOuterCorrectors = int 5 "Number of outer correctors"
//  maxCo = double 5. "Maximum courant number"
//  maxDeltaT = double 1.0 "Maximum time step size"
/*
PARAMETERSET>>> buoyantPimpleFoamNumerics Parameters
inherits FVNumerics::Parameters

time_integration = includedset "insight::CompressiblePIMPLESettings::Parameters" "Settings for time integration"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

    void init();

public:
    declareType ( "buoyantPimpleFoamNumerics" );
    buoyantPimpleFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class FSIDisplacementExtrapolationNumerics
: public FaNumerics
{
public:
#include "numericscaseelements__FSIDisplacementExtrapolationNumerics__Parameters.h"
  
/*
PARAMETERSET>>> FSIDisplacementExtrapolationNumerics Parameters
inherits FaNumerics::Parameters


<<<PARAMETERSET
*/

protected:
  Parameters p_;
   
public:
  FSIDisplacementExtrapolationNumerics( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};




class magneticFoamNumerics
    : public FVNumerics
{

public:
#include "numericscaseelements__magneticFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> magneticFoamNumerics Parameters
inherits FVNumerics::Parameters

solverName = string "magneticFoam" "Name of the solver to use"

<<<PARAMETERSET
*/

protected:
    Parameters p_;
    
    void init();
    
public:
    declareType ( "magneticFoamNumerics" );
    magneticFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




}

#endif // INSIGHT_NUMERICSCASEELEMENTS_H
