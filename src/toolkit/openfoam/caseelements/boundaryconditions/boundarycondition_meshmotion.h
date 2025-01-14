#ifndef BOUNDARYCONDITION_MESHMOTION_H
#define BOUNDARYCONDITION_MESHMOTION_H

#include "base/boost_include.h"
#include "base/parameterset.h"
#include "base/resultset.h"
#include "openfoam/openfoamcase.h"

#include "boundarycondition_meshmotion__MeshMotionBC__Parameters_headers.h"
#include "boundarycondition_meshmotion__CAFSIBC__Parameters_headers.h"


namespace insight
{

namespace MeshMotionBC
{


class MeshMotionBC
{
public:
#include "boundarycondition_meshmotion__MeshMotionBC__Parameters.h"
/*
PARAMETERSET>>> MeshMotionBC Parameters
createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "MeshMotionBC" );
    declareDynamicClass(MeshMotionBC);

    MeshMotionBC(ParameterSetInput ip = ParameterSetInput() );
    virtual ~MeshMotionBC();

    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const =0;
};

typedef std::shared_ptr<MeshMotionBC> MeshMotionBCPtr;


class NoMeshMotion
  : public MeshMotionBC
{
public:
  declareType ( "NoMeshMotion" );

  NoMeshMotion();
  NoMeshMotion ( ParameterSetInput ip );

  bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const override;
};


extern const NoMeshMotion noMeshMotion;


class CAFSIBC
  : public MeshMotionBC
{
public:
#include "boundarycondition_meshmotion__CAFSIBC__Parameters.h"
/*
PARAMETERSET>>> CAFSIBC Parameters
inherits MeshMotionBC::Parameters

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

createGetter
<<<PARAMETERSET
*/


public:
  declareType ( "CAFSIBC" );
  CAFSIBC ( ParameterSetInput ip = ParameterSetInput() );

  void addIntoDictionaries ( OFdicts& dictionaries ) const override;
  bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const override;

};


}

}
#endif // BOUNDARYCONDITION_MESHMOTION_H
