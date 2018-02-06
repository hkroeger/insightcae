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

typedef boost::shared_ptr<HeatBC> HeatBCPtr;

}

}

#endif // BOUNDARYCONDITION_HEAT_H
