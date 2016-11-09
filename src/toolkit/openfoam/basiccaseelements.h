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


#ifndef INSIGHT_BASICCASEELEMENTS_H
#define INSIGHT_BASICCASEELEMENTS_H

#include "openfoam/numericscaseelements.h"
#include "base/linearalgebra.h"
#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"

#include <map>
#include "boost/utility.hpp"
#include "boost/variant.hpp"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight 
{


class gravity
    : public OpenFOAMCaseElement
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (g, arma::mat, vec3(0, 0, -9.81))
//   )

public:
#include "basiccaseelements__gravity__Parameters.h"
/*
PARAMETERSET>>> gravity Parameters

g = vector (0 0 -9.81) "Gravity acceleration"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "gravity" );
    gravity ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};




class volumeDrag
    : public OpenFOAMCaseElement
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (name, std::string, "volumeDrag")
//     (CD, arma::mat, vec3(1, 0, 0))
//   )
public:
#include "basiccaseelements__volumeDrag__Parameters.h"
/*
PARAMETERSET>>> volumeDrag Parameters

name = string "volumeDrag" "Name of the volume drag element"
CD = vector (1 0 0) "Volume drag coefficient for each direction"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "volumeDrag" );
    volumeDrag ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};





class MRFZone
    : public OpenFOAMCaseElement
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (name, std::string, "rotor")
//     (rpm, double, 1000.0)
//     (nonRotatingPatches, std::vector<std::string>, std::vector<std::string>())
//     (rotationCentre, arma::mat, vec3(0,0,0))
//     (rotationAxis, arma::mat, vec3(0,0,1))
//   )
public:
#include "basiccaseelements__MRFZone__Parameters.h"
/*
PARAMETERSET>>> MRFZone Parameters

name = string "rotor" "Name of the MRF zone"
rpm = double 1000.0 "Rotations per minute of the MRF zone"
nonRotatingPatches = array [ string "patchName" "Name of the patch to exclude from rotation" ] *0 "Name of patches to exclude from rotation"
rotationCentre = vector (0 0 0) "Base point of the rotation axis"
rotationAxis = vector (0 0 1) "Direction of the rotation axis"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "MRFZone" );
    MRFZone ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};




class PressureGradientSource
    : public OpenFOAMCaseElement
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (Ubar, arma::mat, vec3(0,0,0))
//   )

public:
#include "basiccaseelements__PressureGradientSource__Parameters.h"
/*
PARAMETERSET>>> PressureGradientSource Parameters

Ubar = vector (0 0 0) "Average velocity"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "PressureGradientSource" );
    PressureGradientSource ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};




class ConstantPressureGradientSource
    : public OpenFOAMCaseElement
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (gradp, arma::mat, vec3(0,0,0))
//   )
public:
#include "basiccaseelements__ConstantPressureGradientSource__Parameters.h"
/*
PARAMETERSET>>> ConstantPressureGradientSource Parameters

gradp = vector (0 0 0) "Constant pressure gradient"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "ConstantPressureGradientSource" );
    ConstantPressureGradientSource ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};




class transportModel
: public OpenFOAMCaseElement
{
public:
  transportModel(OpenFOAMCase& c);
};




class singlePhaseTransportProperties
    : public transportModel
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (nu, double, 1e-6)
//   )
public:
#include "basiccaseelements__singlePhaseTransportProperties__Parameters.h"
/*
PARAMETERSET>>> singlePhaseTransportProperties Parameters

nu = double 1e-6 "Kinematic viscosity"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "singlePhaseTransportProperties" );
    singlePhaseTransportProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};




class twoPhaseTransportProperties
    : public transportModel
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (nu1, double, 1e-6)
//     (rho1, double, 1025.0)
//     (nu2, double, 1.5e-5)
//     (rho2, double, 1.0)
//     (sigma, double, 0.07)
//   )
public:
#include "basiccaseelements__twoPhaseTransportProperties__Parameters.h"
/*
PARAMETERSET>>> twoPhaseTransportProperties Parameters

nu1 = double 1e-6 "Kinematic viscosity of fluid 1"
rho1 = double 1025.0 "Density of fluid 1"
nu2 = double 1.5e-5 "Kinematic viscosity of fluid 2"
rho2 = double 1.0 "Density of fluid 2"
sigma = double 0.07 "Surface tension"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "twoPhaseTransportProperties" );
    twoPhaseTransportProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};




namespace phaseChangeModels
{
  
  
    
  
class phaseChangeModel
{

public:
    declareFactoryTable ( phaseChangeModel, LIST(const ParameterSet& p), LIST(p) );
    declareStaticFunctionTable ( defaultParameters, ParameterSet );
    declareType ( "phaseChangeModel" );

    virtual ~phaseChangeModel();
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const =0;
};



class SchnerrSauer
    : public phaseChangeModel
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (n, double, 1.6e13)
//     (dNuc, double, 2.0e-6)
//     (Cc, double, 1.0)
//     (Cv, double, 1.0)
//   )
public:
#include "basiccaseelements__SchnerrSauer__Parameters.h"
/*
PARAMETERSET>>> SchnerrSauer Parameters

n = double 1.6e13 "n"
dNuc = double 2.0e-6 "dNuc"
Cc = double 1.0 "Cc"
Cv = double 1.0 "Cv"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "SchnerrSauer" );
    SchnerrSauer ( const ParameterSet& p );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};




}




class cavitationTwoPhaseTransportProperties
    : public twoPhaseTransportProperties
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, twoPhaseTransportProperties::Parameters,
//     (model, phaseChangeModels::Ptr,
//       phaseChangeModels::Ptr( new phaseChangeModels::SchnerrSauer() ))
//     (psat, double, 2300.0)
//   )
public:
#include "basiccaseelements__cavitationTwoPhaseTransportProperties__Parameters.h"
/*
PARAMETERSET>>> cavitationTwoPhaseTransportProperties Parameters
inherits twoPhaseTransportProperties::Parameters

psat = double 2300.0 "Saturation pressure"

model = selectableSubset {{ dummy set {} }} dummy "Cavitation model"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "cavitationTwoPhaseTransportProperties" );
    cavitationTwoPhaseTransportProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters();
};




class dynamicMesh
: public OpenFOAMCaseElement
{
public:
  dynamicMesh(OpenFOAMCase& c);
};




class velocityTetFEMMotionSolver
: public dynamicMesh
{
  tetFemNumerics tetFemNumerics_;
public:
  velocityTetFEMMotionSolver(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};




class displacementFvMotionSolver
: public dynamicMesh
{
public:
  displacementFvMotionSolver(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};




}




#endif // INSIGHT_BASICCASEELEMENTS_H
