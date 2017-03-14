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

#include <map>
#include <memory>
#include "boost/utility.hpp"
#include "boost/variant.hpp"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight 
{




/**
 * Interface which wraps different types of prescribing field data on boundaries.
 * Works together with OpenFOAM class "FieldDataProvider".
 */
class FieldData
{
public:

#include "boundaryconditioncaseelements__FieldData__Parameters.h"
  
/*
PARAMETERSET>>> FieldData Parameters

fielddata=selectablesubset {{

 uniform 
 set { 
   values=array
   [
    set {
     time=double 0 "Time instant"
     value=vector (1 0 0) "Field value"
    } "Time instant data"
   ] * 1  "Array of time instants"
 }

 linearProfile 
 set { 
   values=array
   [
    set {
     time=double 0 "Time instant"
     profile=path "profile.dat" "Path to file with tabulated profile for this time instant. Needs to contain one column per component."
    } "Time instant data"
   ] * 1  "Array of time instants"
   
   
   p0=vector (0 0 0) "Origin of sampling axis"
   ep=vector (1 0 0) "Direction of sampling axis"
 }

 radialProfile 
 set { 
   values=array
   [
    set {
     time=double 0 "Time instant"
     profile=path "profile.dat" "Path to file with tabulated profile for this time instant. Needs to contain one column per component."
    } "Time instant data"
   ] * 1  "Array of time instants"
   
   p0=vector (0 0 0) "Origin of sampling axis"
   ep=vector (1 0 0) "Direction of sampling axis"
 }

 fittedProfile 
 set { 
   values=array
   [
    set {
     time=double 0 "Time instant"
     component_coeffs=array
      [
       vector (1 1) "Coefficients of profile polynomial"
      ] * 3 "Sets of polynomial coefficients for each tensor component"
    } "Time instant data"
   ] * 1  "Array of time instants"
   
   p0=vector (0 0 0) "Origin of sampling axis"
   ep=vector (1 0 0) "Direction of sampling axis"
 }

}} uniform "Specification of field value"
<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
    

  static Parameters uniformSteady(double uniformSteadyValue);
  
  /**
   * sets all parameters for the most simple type of field data description (uniform, steady scalar value)
   */
  FieldData(double uniformSteadyValue);
  
  static Parameters uniformSteady(double uniformSteadyX, double uniformSteadyY, double uniformSteadyZ);
  static Parameters uniformSteady(const arma::mat& uniformSteadyValue);

  /**
   * sets all parameters for the most simple type of field data description (uniform, steady value)
   */
  FieldData(const arma::mat& uniformSteadyValue);
  
  /**
   * takes config from a parameterset
   */
  FieldData(const ParameterSet& p);
  
  /**
   * returns according dictionary entry for OF
   */
  OFDictData::data sourceEntry() const;
  
  void setDirichletBC(OFDictData::dict& BC) const;
  
  /**
   * return some representative (average) value of the prescribed data. 
   * Required e.g. for deriving turbulence qtys when velocity distributions are prescribed.
   */
  double representativeValueMag() const;

  /**
   * return the maximum magnitude of the value throughout all precribed times
   */
  double maxValueMag() const;
  
  /**
   * returns a proper parameterset for this entity
   * @reasonable_value: the number of components determines the rank of the field
   */
  static Parameter* defaultParameter(const arma::mat& reasonable_value, const std::string& description="Origin of the prescribed value");
  
  /**
   * insert graphs with prescribed profiles into result set (only, if profiles were prescribed, otherwise inserts nothing)
   * @param results ResultSet to which the data is added
   * @param name Name of result entry
   * @param descr Description
   * @param qtylabel Label (formula symbol) of the quantity. Will be interpreted as latex math expression
   */
  void insertGraphsToResultSet(ResultSetPtr results, const boost::filesystem::path& exepath, const std::string& name, const std::string& descr, const std::string& qtylabel) const;
};
  







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





namespace multiphaseBC
{

    
    
class multiphaseBC;

typedef boost::shared_ptr<multiphaseBC> multiphaseBCPtr;
  
class multiphaseBC
{
public:    
    declareType ( "multiphaseBC" );
    declareDynamicClass(multiphaseBC);
//     declareFactoryTable ( multiphaseBC, LIST ( const ParameterSet& p ), LIST ( p ) );
//     declareStaticFunctionTable ( defaultParameters, ParameterSet );
//     static std::auto_ptr<SelectableSubsetParameter> createSelectableSubsetParameter(const std::string& desc);
//     static multiphaseBCPtr getSelectableSubsetParameter(const SelectableSubsetParameter& ssp);
//     virtual ParameterSet getParameters() const =0;
    
    virtual ~multiphaseBC();
    

    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    // return true, if this field was handled, false otherwise
    virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const =0;
};




class uniformPhases
    : public multiphaseBC
{
// public:
//     typedef std::map<std::string, double> PhaseFractionList;

// protected:
//   PhaseFractionList phaseFractions_;
public:
#include "boundaryconditioncaseelements__uniformPhases__Parameters.h"
/*
PARAMETERSET>>> uniformPhases Parameters

phaseFractions = array [
    set {
    name = string "CO2" "Name of specie"
    fraction = double 0.5 "Mass fraction of specie"
} ] *0 "Mass fractions of species"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "uniformPhases" );
    uniformPhases ( const ParameterSet& p );
    inline static multiphaseBCPtr create(const ParameterSet& ps) { return multiphaseBCPtr(new uniformPhases(ps)); }
    virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const;
    static Parameters mixture( const std::map<std::string, double>& sp);
    static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
    virtual ParameterSet getParameters() const { return p_; }
};

}





namespace turbulenceBC
{
    
class turbulenceBC;

typedef boost::shared_ptr<turbulenceBC> turbulenceBCPtr;
  
class turbulenceBC
{
public:    
    declareType ( "turbulenceBC" );
    declareDynamicClass(turbulenceBC);
//     declareFactoryTable ( turbulenceBC, LIST ( const ParameterSet& p ), LIST ( p ) );
//     declareStaticFunctionTable ( defaultParameters, ParameterSet );
//     static std::auto_ptr<SelectableSubsetParameter> createSelectableSubsetParameter(const std::string& desc);
//     static turbulenceBCPtr getSelectableSubsetParameter(const SelectableSubsetParameter& ssp);
//     virtual ParameterSet getParameters() const =0;

    virtual ~turbulenceBC();
    
//     virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    
    // return true, if this field was handled, false otherwise
//     virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const =0;
    
    virtual void setDirichletBC_k(OFDictData::dict& BC, double U) const =0;
    virtual void setDirichletBC_omega(OFDictData::dict& BC, double U) const =0;
    virtual void setDirichletBC_epsilon(OFDictData::dict& BC, double U) const =0;
    virtual void setDirichletBC_nuTilda(OFDictData::dict& BC, double U) const =0;
    virtual void setDirichletBC_R(OFDictData::dict& BC, double U) const =0;
};





class uniformIntensityAndLengthScale
: public turbulenceBC
{
public:
#include "boundaryconditioncaseelements__uniformIntensityAndLengthScale__Parameters.h"
/*
PARAMETERSET>>> uniformIntensityAndLengthScale Parameters

I = double 0.05 "Fluctuation intensity as fraction of mean velocity"
l = double 0.1 "Length scale"

<<<PARAMETERSET
*/

protected:
    Parameters p_;
    
public:
    declareType("uniformIntensityAndLengthScale");
    uniformIntensityAndLengthScale(const ParameterSet& ps);
    inline static turbulenceBCPtr create(const ParameterSet& ps) { return turbulenceBCPtr(new uniformIntensityAndLengthScale(ps)); }
    
    static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
    virtual ParameterSet getParameters() const { return p_; }
    
    virtual void setDirichletBC_k(OFDictData::dict& BC, double U) const;
    virtual void setDirichletBC_omega(OFDictData::dict& BC, double U) const;
    virtual void setDirichletBC_epsilon(OFDictData::dict& BC, double U) const;
    virtual void setDirichletBC_nuTilda(OFDictData::dict& BC, double U) const;
    virtual void setDirichletBC_R(OFDictData::dict& BC, double U) const;
};



}





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

umean=set {
#include "boundaryconditioncaseelements__FieldData__Parameters.pdl"
} "Mean velocity specification"

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

    R=set {
    #include "boundaryconditioncaseelements__FieldData__Parameters.pdl"
    } "Reynolds stresses specification"

    L=set {
    #include "boundaryconditioncaseelements__FieldData__Parameters.pdl"
    } "Length scale specification"

}

}} uniformIntensityAndLengthScale "Properties of turbulence"

<<<PARAMETERSET
*/


protected:
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
fixMeanValue = bool false "If true, only mean value of pressure is set"
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




namespace MeshMotionBC
{
    
    
class MeshMotionBC;

typedef boost::shared_ptr<MeshMotionBC> MeshMotionBCPtr;

  
class MeshMotionBC
{
public:
    declareType ( "MeshMotionBC" );
    declareDynamicClass(MeshMotionBC);
    
    virtual ~MeshMotionBC();

    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const =0;
};



class NoMeshMotion
  : public MeshMotionBC
{
public:
  declareType ( "NoMeshMotion" );
  NoMeshMotion ( const ParameterSet& ps = ParameterSet() );

  static ParameterSet defaultParameters()
  {
    return ParameterSet();
  }

  virtual ParameterSet getParameters() const
  {
    return ParameterSet();
  }
  
  virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const;

};

extern NoMeshMotion noMeshMotion;


class CAFSIBC
  : public MeshMotionBC
{
public:
#include "boundaryconditioncaseelements__CAFSIBC__Parameters.h"
/*
PARAMETERSET>>> CAFSIBC Parameters

FEMScratchDir = path "" "Directory for data exchange between OF and Code_Aster"
clipPressure = double -100.0 "Lower pressure limit to consider cavitation"
pressureScale = double 1e-3 "Pressure scaling value"

oldPressure = selectablesubset {{

    none set {}

    uniform set { value = double 1e5 "inital pressure value" }

}} none "inital pressure in relaxation process"

relax = selectablesubset {{

    constant set { value = double 0.2 "Constant relaxation factor" }

    profile set { values = array [ set {
    time = double 0 "Time instant"
    value = double 0.2 "Relaxation factor at this instant"
    } ]*1 "time/relaxation factor pairs" }

}} constant "Relaxation"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType ( "CAFSIBC" );
  CAFSIBC ( const ParameterSet& ps );

  static ParameterSet defaultParameters()
  {
    return Parameters::makeDefault();
  }

  virtual ParameterSet getParameters() const
  {
    return p_;
  }

  virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
  virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const;

};


}



class WallBC
    : public BoundaryCondition
{
public:
#include "boundaryconditioncaseelements__WallBC__Parameters.h"
/*
PARAMETERSET>>> WallBC Parameters

wallVelocity = vector (0 0 0) "Velocity of the wlal surface"
rotating = bool false "Whether the wall is rotating"
CofR = vector (0 0 0) "Center of rotation"
roughness_z0 = double 0 "Wall roughness height"
meshmotion = dynamicclassconfig "MeshMotionBC::MeshMotionBC" default "NoMeshMotion" "Mesh motion properties at the boundary"

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
