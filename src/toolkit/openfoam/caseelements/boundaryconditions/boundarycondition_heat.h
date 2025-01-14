#ifndef BOUNDARYCONDITION_HEAT_H
#define BOUNDARYCONDITION_HEAT_H

#include "base/boost_include.h"
#include "base/parameterset.h"
#include "base/resultset.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/fielddata.h"

#include "boundarycondition_heat__FixedTemperatureBC__Parameters_headers.h"

namespace insight
{

namespace HeatBC
{


class HeatBC
{
public:
#include "boundarycondition_heat__HeatBC__Parameters.h"
/*
PARAMETERSET>>> HeatBC Parameters
createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "HeatBC" );
    declareDynamicClass(HeatBC);

    HeatBC(ParameterSetInput ip = ParameterSetInput() );
    virtual ~HeatBC();

    virtual void addOptionsToBoundaryDict ( OFDictData::dict& BCdict ) const;
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual bool addIntoFieldDictionary (
        const std::string& fieldname,
        const FieldInfo& fieldinfo,
        OFDictData::dict& BC,
        OFdicts& dictionaries ) const =0;
};



class AdiabaticBC
  : public HeatBC
{
public:
  declareType ( "Adiabatic" );
  AdiabaticBC ( ParameterSetInput ip = ParameterSetInput() );

  bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, OFdicts& dictionaries ) const override;

};



class FixedTemperatureBC
  : public HeatBC
{
public:
#include "boundarycondition_heat__FixedTemperatureBC__Parameters.h"
/*
PARAMETERSET>>> FixedTemperatureBC Parameters
inherits HeatBC::Parameters

T = includedset "FieldData::Parameters" "Temperature specification"
   modifyDefaults {
    selectablesubset fielddata = uniformSteady;
    vector fielddata/value = 300.0;
   }

createGetter
<<<PARAMETERSET
*/

public:
  declareType ( "FixedTemperature" );
  FixedTemperatureBC ( ParameterSetInput ip = ParameterSetInput() );

  bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, OFdicts& dictionaries ) const override;

};




class TemperatureGradientBC
  : public HeatBC
{
public:
#include "boundarycondition_heat__TemperatureGradientBC__Parameters.h"
/*
PARAMETERSET>>> TemperatureGradientBC Parameters
inherits HeatBC::Parameters

gradT = selectablesubset {{

 constant set {
  gradT = double 0. "[K/m] prescribed temperature gradient"
 }

 unsteady set {
  gradT_vs_t = array [ set {
   t = double 0.0 "time instant"
   gradT = double 0.0 "temperature gradient at that time instant"
  } ]*1 ""
 }
}} constant "specification of temperature gradient"

createGetter
<<<PARAMETERSET
*/

public:
  declareType ( "TemperatureGradientBC" );
  TemperatureGradientBC ( ParameterSetInput ip = ParameterSetInput() );

  bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, OFdicts& dictionaries ) const override;

};





class ExternalWallBC
  : public HeatBC
{

public:
#include "boundarycondition_heat__ExternalWallBC__Parameters.h"
/*
PARAMETERSET>>> ExternalWallBC Parameters
inherits HeatBC::Parameters

kappaSource = selectablesubset {{

 fluidThermo
 set { }

 lookup
 set
 {
   kappaFieldName = string "kappa" "Name of field with thermal conductivity"
 }

}} fluidThermo "Source of thermal conductivity"

heatflux = selectablesubset {{

 fixedPower
 set {
   Q = double 0.0 "[W] Total heat flux"
 }

 fixedHeatFlux
 set {
   q = double 0.0 "[W/m^2] Heat flux per unit area"
 }

 fixedHeatTransferCoeff
 set {
   h = double 10 "[W/m^2/K] Heat transfer coefficient"
   Ta = double 300 "[K] Ambient temperature (beyond domain)"
 }

}} fixedHeatTransferCoeff "Heat flux specification"

wallLayers = array [
 set {
   thickness = double 0.001 "[m] Thickness of layer"
   kappa = double 1 "[W/m/K] Thermal conductivity of layer"
 }
]*1 "Layer composition of external wall"

createGetter
<<<PARAMETERSET
*/

public:
  declareType ( "ExternalWall" );
  ExternalWallBC ( ParameterSetInput ip = ParameterSetInput() );

  bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, OFdicts& dictionaries ) const override;

};


class CHTCoupledWall
  : public HeatBC
{
public:
#include "boundarycondition_heat__CHTCoupledWall__Parameters.h"
/*
PARAMETERSET>>> CHTCoupledWall Parameters
inherits HeatBC::Parameters

sampleRegion = string "" "Neighbouring region" *necessary
samplePatch = string "" "Name of coupled patch in neighbouring region" *necessary
Tnbr = string "T" "Name of temperature field in neighbouring region"

offset = selectablesubset {{
 none set { }
 uniform set {
  distance = vector (0 0 0) "this value is uniformly added to the coordinates of the neighbour patch to make it coincident with the current one." *necessary
 }
}} none "The offset is added to the coordinates of the other neighbour patch to make it conincident with the current patch."

method = selection (nearestPatchFace nearestPatchFaceAMI) nearestPatchFace "The mapping method"

createGetter
<<<PARAMETERSET
*/

public:
  declareType ( "CHTCoupledWall" );
  CHTCoupledWall ( ParameterSetInput ip = ParameterSetInput() );

  void addOptionsToBoundaryDict ( OFDictData::dict& BCdict ) const override;
  bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, OFdicts& dictionaries ) const override;

};

typedef std::shared_ptr<HeatBC> HeatBCPtr;

}

}

#endif // BOUNDARYCONDITION_HEAT_H
