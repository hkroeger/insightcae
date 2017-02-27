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
    : public OpenFOAMCaseElement
{
public:
    declareType ( "FVNumerics" );

public:
#include "numericscaseelements__FVNumerics__Parameters.h"

/*
PARAMETERSET>>> FVNumerics Parameters

np = int 1 "Number of processors"

decompWeights = vector (1 1 1) "Decomposition weights"

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

deltaT = double 1.0 "Time step size"

adjustTimeStep = bool true "Whether to allow time step adjustment during execution"

endTime = double 1000.0 "Maximum end time of simulation"

decompositionMethod = selection ( simple hierarchical metis scotch ) scotch "Parallel decomposition method"

<<<PARAMETERSET
*/

protected:
    Parameters p_;
    bool isCompressible_;

public:
    FVNumerics ( OpenFOAMCase& c, const ParameterSet& ps );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    inline bool isCompressible() const
    {
        return isCompressible_;
    }
    
    static std::string category() { return "Numerics"; }
};




/**
 * create a setDecomposeParDict
 * @poX,@poY,@poZ: define the preference of coordinate directions for decomposition 
 * (relevant for methods simple and hierarchical). 
 * If <0, direction will not be decomposed.
 * Number of decompositions along direction will be ordered according to value of po?
 */
void setDecomposeParDict
(
  OFdicts& dictionaries, 
  int np, 
  const FVNumerics::Parameters::decompositionMethod_type& method,
  const arma::mat& po = vec3(1,1,1)
);



/**
 Manages basic settings in faSchemes, faSolution
 */
class FaNumerics
    : public OpenFOAMCaseElement
{
public:

#include "numericscaseelements__FaNumerics__Parameters.h"

/*
PARAMETERSET>>> FaNumerics Parameters

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    FaNumerics ( OpenFOAMCase& c, const ParameterSet& p = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static std::string category() { return "Numerics"; }
};




/**
 Manages basic settings in tetFemSolution
 */
class tetFemNumerics
    : public OpenFOAMCaseElement
{

public:
    tetFemNumerics ( OpenFOAMCase& c );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static std::string category() { return "Numerics"; }
};



OFDictData::dict diagonalSolverSetup();
OFDictData::dict stdAsymmSolverSetup(double tol=1e-7, double reltol=0.0, int minIter=0);
OFDictData::dict stdSymmSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict smoothSolverSetup(double tol=1e-7, double reltol=0.0, int minIter=0);
OFDictData::dict GAMGSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict GAMGPCGSolverSetup(double tol=1e-7, double reltol=0.0);




class MeshingNumerics
    : public FVNumerics
{
public:
    declareType ( "MeshingNumerics" );

    MeshingNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    
    static ParameterSet defaultParameters();
};




class simpleFoamNumerics
    : public FVNumerics
{
    
public:
#include "numericscaseelements__simpleFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> simpleFoamNumerics Parameters
inherits FVNumerics::Parameters

checkResiduals = bool true "Whether to check residuals during run"
hasCyclics = bool false "Whether the model contains cyclic boundaries"
pinternal = double 0.0 "Internal pressure field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "simpleFoamNumerics" );
    simpleFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class pimpleFoamNumerics
    : public FVNumerics
{

public:
#include "numericscaseelements__pimpleFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> pimpleFoamNumerics Parameters
inherits FVNumerics::Parameters

nCorrectors = int 2 "Number of correctors"
nOuterCorrectors = int 1 "Number of outer correctors"
nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"
maxCo = double 0.45 "Maximum courant number"
maxDeltaT = double 1.0 "Maximum time step size"
forceLES = bool false "Whether to enforce LES numerics"
LESfilteredConvection = bool false "Whether to use filtered linear convection schemes instead of linear when using LES"
hasCyclics = bool false "Whether the model contains cyclic boundaries"
pinternal = double 0.0 "Internal pressure field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "pimpleFoamNumerics" );
    pimpleFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class potentialFreeSurfaceFoamNumerics
    : public FVNumerics
{

public:
#include "numericscaseelements__potentialFreeSurfaceFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> potentialFreeSurfaceFoamNumerics Parameters
inherits FVNumerics::Parameters

nCorrectors = int 2 "Number of correctors"
nOuterCorrectors = int 1 "Number of outer correctors"
nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"
maxCo = double 0.45 "Maximum courant number"
maxDeltaT = double 1.0 "Maximum time step size"

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

/*
PARAMETERSET>>> interFoamNumerics Parameters
inherits FVNumerics::Parameters

implicitPressureCorrection = bool false "Whether to switch to implicit pressure correction"
nOuterCorrectors = int 50 "Number of outer correctors"

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




OFDictData::dict stdMULESSolverSetup(double tol=1e-8, double reltol=0.0, bool LTS=false);




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
pamb = double 1e5 "Ambient pressure"

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

/*
PARAMETERSET>>> reactingFoamNumerics Parameters
inherits FVNumerics::Parameters


nCorrectors = int 2 "Number of correctors"
nOuterCorrectors = int 1 "Number of outer correctors"
nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"
maxCo = double 0.45 "Maximum courant number"
maxDeltaT = double 1.0 "Maximum time step size"
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
