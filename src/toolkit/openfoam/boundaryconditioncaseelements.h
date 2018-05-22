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


#ifndef INSIGHT_BOUNDARYCONDITIONCASEELEMENTS_H
#define INSIGHT_BOUNDARYCONDITIONCASEELEMENTS_H

#include "openfoam/numericscaseelements.h"
#include "base/linearalgebra.h"
#include "base/parameterset.h"
#include "base/resultset.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/fielddata.h"
#include "openfoam/boundarycondition_heat.h"
#include "openfoam/boundarycondition_meshmotion.h"
#include "openfoam/boundarycondition_multiphase.h"
#include "openfoam/boundarycondition_turbulence.h"

#include <map>
#include <memory>
#include "boost/utility.hpp"
#include "boost/variant.hpp"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight 
{










class SimpleBC
: public BoundaryCondition
{
    
public:
#include "boundaryconditioncaseelements__SimpleBC__Parameters.h"
/*
PARAMETERSET>>> SimpleBC Parameters

className = string "empty" "Class name of the boundary condition."

<<<PARAMETERSET
*/

protected:
    Parameters p_;
  
    void init();
    
public:
    declareType("SimpleBC");
  SimpleBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const std::string className);
  SimpleBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& p);
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
  
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
};




class SymmetryBC
: public SimpleBC
{

public:
  declareType("SymmetryBC");
  SymmetryBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& p = ParameterSet() );
  static ParameterSet defaultParameters() { return ParameterSet(); }
};




class EmptyBC
: public SimpleBC
{

public:
  declareType("EmptyBC");
  EmptyBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& p = ParameterSet() );
  static ParameterSet defaultParameters() { return ParameterSet(); }
};




class CyclicPairBC
    : public OpenFOAMCaseElement
{

protected:
    std::string patchName_;
    int nFaces_, nFaces1_;
    int startFace_, startFace1_;

public:
    declareType ( "CyclicPairBC" );
    CyclicPairBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& p = ParameterSet() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;

    virtual bool providesBCsForPatch ( const std::string& patchName ) const;

    static ParameterSet defaultParameters()
    {
        return ParameterSet();
    }

};




class GGIBCBase
    : public BoundaryCondition
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (shadowPatch, std::string, "")
//     (bridgeOverlap, bool, true)
//     (zone, std::string, "")
//   )
public:
#include "boundaryconditioncaseelements__GGIBCBase__Parameters.h"
/*
PARAMETERSET>>> GGIBCBase Parameters

shadowPatch = string "" "Name of the opposite patch"
zone = string "" "Zone name. Usually equal to patch name."
bridgeOverlap = bool true "Whether to fix small non-overlapping areas."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    GGIBCBase ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
                const ParameterSet&ps = Parameters::makeDefault() );
    virtual void modifyMeshOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const;
};




class GGIBC
    : public GGIBCBase
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, GGIBCBase::Parameters,
//     (separationOffset, arma::mat, vec3(0,0,0))
//   )
public:
#include "boundaryconditioncaseelements__GGIBC__Parameters.h"
/*
PARAMETERSET>>> GGIBC Parameters
inherits GGIBCBase::Parameters

separationOffset = vector (0 0 0) "Translational transformation from this patch to the opposite one."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "GGIBC" );
    GGIBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
            const ParameterSet&ps = Parameters::makeDefault() );
    virtual void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
};




class CyclicGGIBC
    : public GGIBCBase
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, GGIBCBase::Parameters,
//     (separationOffset, arma::mat, vec3(0,0,0))
//     (rotationCentre, arma::mat, vec3(0,0,0))
//     (rotationAxis, arma::mat, vec3(0,0,1))
//     (rotationAngle, double, 0.0)
//   )
public:
#include "boundaryconditioncaseelements__CyclicGGIBC__Parameters.h"
/*
PARAMETERSET>>> CyclicGGIBC Parameters
inherits GGIBCBase::Parameters

separationOffset = vector (0 0 0) "Translational transformation from this patch to the opposite one."
rotationCentre = vector (0 0 0) "Origin of rotation axis"
rotationAxis = vector (0 0 1) "Direction of rotation axis"
rotationAngle = double 0.0 "[rad] Angle of rotation"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "CyclicGGIBC" );
    CyclicGGIBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
                  const ParameterSet&ps = Parameters::makeDefault() );
    virtual void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
};




class OverlapGGIBC
    : public GGIBCBase
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, GGIBCBase::Parameters,
//     (separationOffset, arma::mat, vec3(0,0,0))
//     (rotationAxis, arma::mat, vec3(0,0,1))
//     (nCopies, int, 1)
//   )
public:
#include "boundaryconditioncaseelements__OverlapGGIBC__Parameters.h"
/*
PARAMETERSET>>> OverlapGGIBC Parameters
inherits GGIBCBase::Parameters

separationOffset = vector (0 0 0) "Translational transformation from this patch to the opposite one."
rotationAxis = vector (0 0 1) "Direction of rotation axis"
nCopies = int 1 "number of copies"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "OverlapGGIBC" );
    OverlapGGIBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
                   const ParameterSet&ps = Parameters::makeDefault() );
    virtual void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
};




class MixingPlaneGGIBC
    : public GGIBCBase
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, GGIBCBase::Parameters,
//     (separationOffset, arma::mat, vec3(0,0,0))
//   )
public:
#include "boundaryconditioncaseelements__MixingPlaneGGIBC__Parameters.h"
/*
PARAMETERSET>>> MixingPlaneGGIBC Parameters
inherits GGIBCBase::Parameters

separationOffset = vector (0 0 0) "Translational transformation from this patch to the opposite one."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "MixingPlaneGGIBC" );
    MixingPlaneGGIBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
                       const ParameterSet&ps = Parameters::makeDefault() );
    virtual void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
};












class SuctionInletBC
    : public BoundaryCondition
{
public:
#include "boundaryconditioncaseelements__SuctionInletBC__Parameters.h"
/*
PARAMETERSET>>> SuctionInletBC Parameters

pressure = double 0.0 "Total pressure at boundary"
rho = double 1025.0 "Density at boundary"
T = double 300.0 "Temperature at boundary"
gamma = double 1.0 "Ratio of specific heats at boundary"
phiName = string "phi" "Name of flux field"
psiName = string "none" "Name of compressibility field"
rhoName = string "none" "Name of density field"
UName = string "U" "Name of velocity field"

turb_I = double 0.05 "[-] turbulence intensity at inflow"
turb_L = double 0.1 "[m] turbulent length scale at inflow"

phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType ( "SuctionInletBC" );
    SuctionInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet&ps = Parameters::makeDefault()
    );
    
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;
    
    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};






class MassflowBC
    : public BoundaryCondition
{
public:
#include "boundaryconditioncaseelements__MassflowBC__Parameters.h"
/*
PARAMETERSET>>> MassflowBC Parameters

massflow = double 1.0 "mass flow through boundary"
rho = double 1025.0 "Density at boundary"
T = double 300.0 "Temperature at boundary"
gamma = double 1.0 "Ratio of specific heats at boundary"
phiName = string "phi" "Name of flux field"
psiName = string "none" "Name of compressibility field"
rhoName = string "none" "Name of density field"
UName = string "U" "Name of velocity field"
turbulence = dynamicclassconfig "turbulenceBC::turbulenceBC" default "uniformIntensityAndLengthScale" "Definition of the turbulence state at the boundary"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType ( "MassflowBC" );
    MassflowBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& ps = Parameters::makeDefault()
    );
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();;
    }

};


class MappedVelocityInletBC
    : public BoundaryCondition
{
public:
#include "boundaryconditioncaseelements__MappedVelocityInletBC__Parameters.h"
/*
PARAMETERSET>>> MappedVelocityInletBC Parameters

distance = vector (1 0 0) "distance of sampling plane"
average =  vector (1 0 0) "average"
rho = double 1025.0 "Density at boundary"
T = double 300.0 "Temperature at boundary"
gamma = double 1.0 "Ratio of specific heats at boundary"
phiName = string "phi" "Name of flux field"
psiName = string "none" "Name of compressibility field"
rhoName = string "none" "Name of density field"
UName = string "U" "Name of velocity field"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType ( "MappedVelocityInletBC" );
    MappedVelocityInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet&ps = Parameters::makeDefault()
    );
    
    virtual void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const;    
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;
    
    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};


/**
 * The base class for inlet boundaries in OpenFOAM.
 * It handles all informations for the creation of a dirichlet BC in the velocity field (mean velocity in turbulent cases).
 * No handling of turbulence at this stage.
 */
class VelocityInletBC
    : public BoundaryCondition
{
public:
#include "boundaryconditioncaseelements__VelocityInletBC__Parameters.h"
/*
PARAMETERSET>>> VelocityInletBC Parameters

velocity = includedset "FieldData::Parameters" "Velocity specification"
T = includedset "FieldData::Parameters" "Temperature at boundary"
rho = includedset "FieldData::Parameters" "Density at boundary"
turbulence = dynamicclassconfig "turbulenceBC::turbulenceBC" default "uniformIntensityAndLengthScale" "Definition of the turbulence state at the boundary"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType("VelocityInletBC");
    
    VelocityInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& p = Parameters::makeDefault()
    );

    virtual void setField_U ( OFDictData::dict& BC ) const;
    virtual void setField_p ( OFDictData::dict& BC ) const;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};





class ExptDataInletBC
    : public BoundaryCondition
{
public:
#include "boundaryconditioncaseelements__ExptDataInletBC__Parameters.h"
/*
PARAMETERSET>>> ExptDataInletBC Parameters

data = array[ set {
    point = vector (0 0 0) "Point coordinate"
    velocity = vector (0 0 0) "Velocity at this point"
    k = double 0.1 "Turbulent kinetic energy at this point"
    epsilon = double 0.1 "Turbulent dissipation rate"
} ] *1 "Velocity specification per point"

phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType("ExptDataInletBC");
    
    ExptDataInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& p = Parameters::makeDefault()
    );
    
    virtual void addDataDict ( OFdicts& dictionaries, const std::string& prefix, const std::string& fieldname, const arma::mat& data ) const;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};





class CompressibleInletBC
    : public VelocityInletBC
{
public:
#include "boundaryconditioncaseelements__CompressibleInletBC__Parameters.h"
/*
PARAMETERSET>>> CompressibleInletBC Parameters
inherits VelocityInletBC::Parameters

pressure = double 1e5 "Static pressure at the inlet"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType ( "CompressibleInletBC" );
    
    CompressibleInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& p = Parameters::makeDefault()
    );
    virtual void setField_p ( OFDictData::dict& BC ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }

};





class TurbulentVelocityInletBC
    : public BoundaryCondition
{

public:

    static const std::vector<std::string> inflowGenerator_types;

#include "boundaryconditioncaseelements__TurbulentVelocityInletBC__Parameters.h"
/*
PARAMETERSET>>> TurbulentVelocityInletBC Parameters

umean = includedset "FieldData::Parameters" "Mean velocity specification"

turbulence = selectablesubset {{

uniformIntensityAndLengthScale
set {
    intensity = double 0.05 "Turbulence intensity as fraction of mean velocity"
    lengthScale = double 0.1 "[m] Turbulence length scale"
}

inflowGenerator
set {
    uniformConvection=bool false "Whether to use a uniform convection velocity instead of the local mean velocity"

    volexcess=double 2.0 "Volumetric overlapping of spots"

    type=selection (
    hatSpot
    gaussianSpot
    decayingTurbulenceSpot
    decayingTurbulenceVorton
    anisotropicVorton_Analytic
    anisotropicVorton_PseudoInv
    anisotropicVorton_NumOpt
    anisotropicVorton2
    combinedVorton
    modalTurbulence
    ) anisotropicVorton "Type of inflow generator"

    R=includedset "FieldData::Parameters" "Reynolds stresses specification"

    L=includedset "FieldData::Parameters" "Length scale specification"

}

}} uniformIntensityAndLengthScale "Properties of turbulence"

phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/


protected:
    ParameterSet ps_;
    Parameters p_;

public:
    declareType ( "TurbulentVelocityInletBC" );
    TurbulentVelocityInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& p = Parameters::makeDefault()
    );

    virtual void setField_U ( OFDictData::dict& BC ) const;
    virtual void setField_p ( OFDictData::dict& BC ) const;
    virtual void setField_k ( OFDictData::dict& BC ) const;
    virtual void setField_epsilon ( OFDictData::dict& BC ) const;
    virtual void setField_omega ( OFDictData::dict& BC ) const;
    virtual void setField_nuTilda ( OFDictData::dict& BC ) const;
    virtual void setField_R ( OFDictData::dict& BC ) const;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;

    inline static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};





class PressureOutletBC
    : public BoundaryCondition
{
public:
#include "boundaryconditioncaseelements__PressureOutletBC__Parameters.h"
/*
PARAMETERSET>>> PressureOutletBC Parameters

pressure = double 0.0 "Uniform static pressure at selected boundary patch"
prohibitInflow = bool true "Whether to clip velocities to zero in case of flow reversal"

behaviour = selectablesubset {{

 uniform
 set { }
 
 fixMeanValue
 set { }

 waveTransmissive 
 set {
  kappa = double 1.4 "Specific heat ratio"
  L = double 1 "Reference length"
 }

 removePRGHHydrostaticPressure
 set { }
 
}} uniform "Behaviour of the pressure BC"

rho = double 1025.0 "Density"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType ( "PressureOutletBC" );
    PressureOutletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& ps = Parameters::makeDefault()
    );
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;
    
    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};





class PotentialFreeSurfaceBC
: public BoundaryCondition
{
public:
  PotentialFreeSurfaceBC
  (
    OpenFOAMCase& c, 
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict
  );
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};







class WallBC
    : public BoundaryCondition
{
public:
#include "boundaryconditioncaseelements__WallBC__Parameters.h"
/*
PARAMETERSET>>> WallBC Parameters

wallVelocity = vector (0 0 0) "Velocity of the wall surface"
rotating = bool false "Whether the wall is rotating"
CofR = vector (0 0 0) "Center of rotation"
roughness_z0 = double 0 "Wall roughness height"
meshmotion = dynamicclassconfig "MeshMotionBC::MeshMotionBC" default "NoMeshMotion" "Mesh motion properties at the boundary"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"
heattransfer = dynamicclassconfig "HeatBC::HeatBC" default "Adiabatic" "Definition of the heat transfer through the wall"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType("WallBC");
    
    WallBC
    (
        OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
        const ParameterSet &ps = Parameters::makeDefault()
    );

    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual void addIntoFieldDictionaries ( OFdicts& dictionaries ) const;
    virtual void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }

};




}

#endif // INSIGHT_BASICCASEELEMENTS_H
