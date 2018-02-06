#ifndef BOUNDARYCONDITION_HEAT_H
#define BOUNDARYCONDITION_HEAT_H

#include "base/boost_include.h"
#include "base/parameterset.h"
#include "base/resultset.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/fielddata.h"



namespace insight
{

namespace HeatBC
{


class HeatBC
{
public:
    declareType ( "HeatBC" );
    declareDynamicClass(HeatBC);

    virtual ~HeatBC();

    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const =0;
};



class AdiabaticBC
  : public HeatBC
{
public:
  declareType ( "Adiabatic" );
  AdiabaticBC ( const ParameterSet& ps = ParameterSet() );

  static ParameterSet defaultParameters() { return ParameterSet(); }
  virtual ParameterSet getParameters() const { return ParameterSet(); }

  virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const;

};



class FixedTemperatureBC
  : public HeatBC
{
public:
#include "boundarycondition_heat__FixedTemperatureBC__Parameters.h"
/*
PARAMETERSET>>> FixedTemperatureBC Parameters

T = includedset "FieldData::Parameters" "Temperature specification"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType ( "FixedTemperature" );
  FixedTemperatureBC ( const ParameterSet& ps = ParameterSet() );

  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }

  virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const;

};




class ExternalWallBC
  : public HeatBC
{

public:
#include "boundarycondition_heat__ExternalWallBC__Parameters.h"
/*
PARAMETERSET>>> ExternalWallBC Parameters

heatflux = selectablesubset {{

 constant
 set {
   q = double 0.0 "[W/m^2] Heat flux per unit area"
 }

 convective
 set {
   h = double 10 "[W/m^2/K] Heat transfer coefficient"
   Ta = double 300 "[K] Ambient temperature (beyond domain)"
 }

}} convective "Heat flux specification"

wallLayers = array [
 set {
   thickness = double 0.001 "[m] Thickness of layer"
   kappa = double 1 "[W/m/K] Thermal conductivity of layer"
 }
]*1 "Layer composition of external wall"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType ( "ExternalWall" );
  ExternalWallBC ( const ParameterSet& ps = ParameterSet() );

  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }

  virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const;

};



typedef boost::shared_ptr<HeatBC> HeatBCPtr;

}

}

#endif // BOUNDARYCONDITION_HEAT_H
