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

#include "openfoam/caseelements/numericscaseelements.h"
#include "base/linearalgebra.h"
#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"

#include <map>
#include "boost/utility.hpp"
#include "boost/variant.hpp"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight 
{

class cellSetOption_Selection
{
public:
#include "basiccaseelements__cellSetOption_Selection__Parameters.h"
/*
PARAMETERSET>>> cellSetOption_Selection Parameters

selection = selectablesubset {{
 all set {}
 cellZone set {
  zoneName = string "zone" "Name of the cell zone"
 }
 cellSet set {
  setName = string "set" "Name of the cell set"
 }
}} all "Selection of cells"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  cellSetOption_Selection(const Parameters& p);

  void insertSelection(OFDictData::dict& d);
};


class gravity
    : public OpenFOAMCaseElement
{

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
    virtual bool isUnique() const;

    static ParameterSet defaultParameters();
    static std::string category();
};



class mirrorMesh
    : public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__mirrorMesh__Parameters.h"
/*
PARAMETERSET>>> mirrorMesh Parameters

plane = selectablesubset {{
 pointAndNormal set {
  p0 = vector (0 0 0) "Origin point"
  normal = vector (0 0 1) "plane normal"
 }

 threePoint set {
  p0 = vector (0 0 0) "First point"
  p1 = vector (1 0 0) "Second point"
  p2 = vector (0 1 0) "Third point"
 }

 }} pointAndNormal "Mirror plane definition"

planeTolerance = double 1e-3 "plane tolerance"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "mirrorMesh" );
    mirrorMesh ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual bool isUnique() const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
    static std::string category() { return "Meshing"; }
};




class setFieldsConfiguration
    : public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__setFieldsConfiguration__Parameters.h"
/*
PARAMETERSET>>> setFieldsConfiguration Parameters

defaultValues = array [ selectablesubset {{

  scalar set {
   name = string "alpha.phase1" "Name of the field"
   value = double 0 "default value"
  }

  vector set {
   name = string "U" "Name of the field"
   value = vector (0 0 0) "default value"
  }

}} scalar ] *1 "default field values (in regions not covered by region selectors)"



regionSelectors = array [ selectablesubset {{

  box set {
   p0 = vector (-1e10 -1e10 -1e10) "Minimum corner of the box"
   p1 = vector (1e10 1e10 1e10) "Maximum corner of the box"

   selectfaces = bool true "check to select faces"
   selectcells = bool true "check to select cells"

   regionValues = array [ selectablesubset {{

      scalar set {
       name = string "alpha.phase1" "Name of the field"
       value = double 0 "field value"
      }

      vector set {
       name = string "U" "Name of the field"
       value = vector (0 0 0) "field value"
      }

    }} scalar ] *1 "field values in selected region"

  }

 }} box ]*1 "region selectors"


<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "setFieldsConfiguration" );
    setFieldsConfiguration ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual bool isUnique() const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
    static std::string category() { return "Preprocessing"; }
};




class volumeDrag
    : public OpenFOAMCaseElement
{

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
    static std::string category() { return "Body Force"; }
};





class fixedValueConstraint
: public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__fixedValueConstraint__Parameters.h"
/*
PARAMETERSET>>> fixedValueConstraint Parameters

name = string "fixedValueConstraint" "Name of the volume drag element"
fieldName = string "U" "Name of the field which shall be constrained"
zoneName = string "" "Name of the zone with the cells whose value shall be constrained"

value = selectablesubset {{
 scalar set {
  value = double 0.0 "value inside zone"
 }

 vector set {
  value = vector (0 0 0) "vector value inside zone"
 }

}} vector "value inside zone"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "fixedValueConstraint" );
    fixedValueConstraint ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
    static std::string category() { return "Body Force"; }
};



class source
: public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__source__Parameters.h"
/*
PARAMETERSET>>> source Parameters

name = string "source" "Name of the volume drag element"
fieldName = string "T" "Name of the field to which this source shall contribute"
zoneName = string "" "Name of the zone with the cells which receive the source contribution"

volumeMode = selection (absolute specific) absolute
"Whether the value is specific or not. If absolute, the unit is that of the base field. If specific, it is that of the base field divided by $m^3$."

value = selectablesubset {{
 scalar set {
  value_const = double 0.0 "constant contribution"
  value_lin = double 0.0 "contribution propotional to base field value"
 }

 vector set {
  value_const = vector (0 0 0) "constant contribution"
  value_lin = vector (0 0 0) "contribution propotional to base field value"
 }

}} scalar "value of the source"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "source" );
    source ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
    static std::string category() { return "Body Force"; }
};


class MRFZone
    : public OpenFOAMCaseElement
{

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
    static std::string category() { return "Rotation"; }
};



class PassiveScalar
    : public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__PassiveScalar__Parameters.h"
/*
PARAMETERSET>>> PassiveScalar Parameters

fieldname = string "F" "Name of the passive scalar field"
internal = double 0.0 "Default internal value"

underrelax = double 1.0 "Underrelaxation factor"

scheme = selectablesubset {{
 firstorder set {}
 secondorder set {
   bounded01 = bool true "If enabled, a 01 bounded scheme will be used for interpolation."
 }
}} secondorder "Accuracy/stability trade off"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "PassiveScalar" );
    PassiveScalar ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addFields( OpenFOAMCase& c ) const;
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
    static std::string category() { return "Numerics"; }
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
    static std::string category() { return "Body Force"; }
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
    static std::string category() { return "Body Force"; }
};




class transportModel
: public OpenFOAMCaseElement
{
public:
  transportModel(OpenFOAMCase& c);
  virtual bool isUnique() const;
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
    static std::string category() { return "Material Properties"; }
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
    static std::string category() { return "Material Properties"; }
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

model = selectablesubset {{ dummy set { } }} dummy "Cavitation model"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_; // need to use dynamic variant; will contain enhancements to above definition

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
  static std::string category() { return "Dynamic Mesh"; }
  virtual bool isUnique() const;
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




class solidBodyMotionDynamicMesh
: public dynamicMesh
{
public:
#include "basiccaseelements__solidBodyMotionDynamicMesh__Parameters.h"
/*
PARAMETERSET>>> solidBodyMotionDynamicMesh Parameters

zonename = string "none" "Name of the cell zone which moves.
Enter 'none', if the entire mesh shall be moved."

motion = selectablesubset
{{
 
 rotation
 set {
  origin = vector (0 0 0) "origin point"
  axis = vector (0 0 1) "rotation axis"
  rpm = double 1000 "rotation rate"
 }

 oscillatingRotating
 set {
  origin = vector (0 0 0) "origin point"
  amplitude = vector (0 0 1) "[deg] amplitude"
  omega = double 1 "[rad/sec] rotation frequency"
 }

}} rotation "type of motion"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_; // need to use dynamic variant; will contain enhancements to above definition

public:
  declareType ( "solidBodyMotionDynamicMesh" );
  
  solidBodyMotionDynamicMesh( OpenFOAMCase& c, const ParameterSet&ps = Parameters::makeDefault() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  
  static ParameterSet defaultParameters()
  {
      return Parameters::makeDefault();
  }
  static std::string category() { return "Dynamic Mesh"; }
};





class rigidBodyMotionDynamicMesh
: public dynamicMesh
{
public:
#include "basiccaseelements__rigidBodyMotionDynamicMesh__Parameters.h"
/*
PARAMETERSET>>> rigidBodyMotionDynamicMesh Parameters

rho = selectablesubset {{
 field set {
  fieldname = string "rho" "Density field name"
 }
 constant set {
  rhoInf = double 1025.0 "Constant density value"
 }
}} constant "Density source"


bodies = array [
 set {
   name = string "movingbody" "Name of the body"
   centreOfMass = vector (0 0 0) "Location of CoG in global CS"
   mass = double 1.0 "Mass of body"
   Ixx = double 1.0 "Inertia Ixx"
   Iyy = double 1.0 "Inertia Iyy"
   Izz = double 1.0 "Inertia Izz"

   patches = array [
    string "bodysurface" "Names of patches comprising the surface of the body"
   ] *1 "body surface patches"

   innerDistance = double 1.0 "radius around body within which a solid body motion is performed."
   outerDistance = double 1.0 "radius around body outside which the grid remains fixed."

   translationConstraint = array [ selection (
    Px Py Pz Pxyz ) Pxyz "Kind of translation constraint"
   ] *1 "translation constraints"

   rotationConstraint = array [ selection (
    Rx Ry Rz Rxyz ) Rxyz "Kind of rotation constraint"
   ] *1 "rotation constraints"

 } ] *1 "moving bodies"


<<<PARAMETERSET
*/

protected:
    ParameterSet ps_; // need to use dynamic variant; will contain enhancements to above definition

public:
  declareType ( "rigidBodyMotionDynamicMesh" );

  rigidBodyMotionDynamicMesh( OpenFOAMCase& c, const ParameterSet&ps = Parameters::makeDefault() );
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;

  static ParameterSet defaultParameters()
  {
      return Parameters::makeDefault();
  }
  static std::string category() { return "Dynamic Mesh"; }
};





class porousZone
    : public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__porousZone__Parameters.h"
/*
PARAMETERSET>>> porousZone Parameters

name = string "porosity" "Name of the porous cell zone. It needs to exist for this configuration to work."
d = vector (1 1 1) "Darcy coefficients for each direction"
f = vector (0 0 0) "Forchheimer coefficients for each direction"

direction_x = vector (1 0 0) "X direction of the porosity coordinate system"
direction_y = vector (0 1 0) "Y direction of the porosity coordinate system"
<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "porousZone" );
    porousZone ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
    static std::string category() { return "Body Force"; }
};





class limitQuantities
    : public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__limitQuantities__Parameters.h"
/*
PARAMETERSET>>> limitQuantities Parameters

name = string "limitQty" "Name prefix of the fvOptions"

cells = includedset "cellSetOption_Selection::Parameters" "selection of the cells, inside which the quantities are limited"

limitTemperature = selectablesubset {{
 none set {}
 limit set {
  min = double 0 "lower limit of temperature to enforce"
  max = double 1e10 "upper limit of temperature to enforce"
 }
}} limit "Type of temperature limit"

limitVelocity = selectablesubset {{
 none set {}
 limit set {
  max = double 1e10 "upper limit of velocity magnitude"
 }
}} limit "Type of temperature limit"

limitFields = array [ set {
 fieldName = string "p_rgh" "Name of field to limit"
 type = selection (scalar vector symmTensor tensor) scalar "Type of field"
 min = double -1e10 "Minimum value (magnitude for non-scalar fields)"
 max = double 1e10 "Maximum value (magnitude for non-scalar fields)"
} ] *0 "Fields to limit"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "limitQuantities" );
    limitQuantities ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
    static std::string category() { return "Tweaks"; }
};




class customDictEntries
: public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__customDictEntries__Parameters.h"

/*
PARAMETERSET>>> customDictEntries Parameters

entries = array [ set {
 dict = string "system/controlDict" "Path to dictionary"
 path = string "endTime" "Key or path to key. Use slashes to access subdicts. Non-existing subdicts will be created."
 value = string "1000" "Value to set (verbatim string copy will be copied)"
} ] *0 "Array of entries to set"

appendList = array [ set {
 dict = string "system/controlDict" "Path to dictionary"
 path = string "libs" "Key or path to list. Use slashes to access subdicts. Non-existing subdicts will be created."
 value = string "libSRFoption.so" "Value to append (verbatim string copy will be copied)"
} ] *0 "Array of entries to append to lists"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType("customDictEntries");
  customDictEntries(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;

  static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }

  static std::string category() { return "Custom"; }
};



class copyFiles
: public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__copyFiles__Parameters.h"

/*
PARAMETERSET>>> copyFiles Parameters

files = array [ set {
 source = path "" "File or directory which shall be copied into the case."
 target = string "constant/polyMesh" "Target location (relative to case directory)"
} ] *0 "Array of copy file operations"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType("copyFiles");
  copyFiles(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  virtual void modifyFilesOnDiskBeforeDictCreation ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const;

  static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }

  static std::string category() { return "Custom"; }
};




class SRFoption
: public OpenFOAMCaseElement
{

public:
#include "basiccaseelements__SRFoption__Parameters.h"

/*
PARAMETERSET>>> SRFoption Parameters

origin = vector (0 0 0) "Center of the rotating motion"
axis = vector (0 0 1) "Axis of rotation"
rpm = double 1000.0 "Number of revolutions per minute"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType("SRFoption");
  SRFoption(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;

  static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }

  static std::string category() { return "Body Force"; }
};


}




#endif // INSIGHT_BASICCASEELEMENTS_H
