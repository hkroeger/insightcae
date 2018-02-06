#ifndef BOUNDARYCONDITION_MESHMOTION_H
#define BOUNDARYCONDITION_MESHMOTION_H

#include "base/boost_include.h"
#include "base/parameterset.h"
#include "base/resultset.h"
#include "openfoam/openfoamcase.h"


namespace insight
{

namespace MeshMotionBC
{


class MeshMotionBC
{
public:
    declareType ( "MeshMotionBC" );
    declareDynamicClass(MeshMotionBC);

    virtual ~MeshMotionBC();

    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const =0;
};

typedef boost::shared_ptr<MeshMotionBC> MeshMotionBCPtr;


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
#include "boundarycondition_meshmotion__CAFSIBC__Parameters.h"
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

}
#endif // BOUNDARYCONDITION_MESHMOTION_H
