#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"


#include <string>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::fusion;


namespace insight
{
namespace MeshMotionBC
{


const PassiveMeshMotion passiveMeshMotion;




defineType(MeshMotionBC);
defineDynamicClass(MeshMotionBC);

MeshMotionBC::MeshMotionBC(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}

MeshMotionBC::~MeshMotionBC()
{}


void MeshMotionBC::addIntoDictionaries(OFdicts&) const
{}



defineType(PassiveMeshMotion);
addToFactoryTable(MeshMotionBC, PassiveMeshMotion);
addToStaticFunctionTable(MeshMotionBC, PassiveMeshMotion, defaultParameters);

PassiveMeshMotion::PassiveMeshMotion()
    : MeshMotionBC(Parameters())
{}

PassiveMeshMotion::PassiveMeshMotion(ParameterSetInput ip)
    : MeshMotionBC(ip.forward<Parameters>())
{}

bool PassiveMeshMotion::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
    if
    (
      ((fieldname=="pointDisplacement")||(fieldname == "motionU"))
      &&
      (get<0>(fieldinfo)==vectorField)
    )
    {
      BC["type"]=OFDictData::data("calculated");
      BC["value"]=OFDictData::toUniformField(vec3Zero());
      return true;
    }
    else
      return false;
}






defineType(CAFSIBC);
addToFactoryTable(MeshMotionBC, CAFSIBC);
addToStaticFunctionTable(MeshMotionBC, CAFSIBC, defaultParameters);

CAFSIBC::CAFSIBC(ParameterSetInput ip)
: MeshMotionBC(ip.forward<Parameters>())
{
}


void CAFSIBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libFEMDisplacementBC.so\"") );
}

bool CAFSIBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& /*fieldinfo*/, OFDictData::dict& BC) const
{
  if (fieldname == "motionU")
  {
    BC["prescribeMotionVelocity"] = OFDictData::data(true);
  }
  if ( (fieldname == "pointDisplacement") || (fieldname == "motionU") )
  {
    BC["type"]= OFDictData::data("FEMDisplacement");
    BC["FEMCaseDir"]=  OFDictData::data(std::string("\"")+p().FEMScratchDir->expandedFilePath().string()+"\"");
    BC["pressureScale"]=  OFDictData::data(p().pressureScale);
    BC["minPressure"]=  OFDictData::data(p().clipPressure);
    BC["nSmoothIter"]=  OFDictData::data(4);
    BC["wallCollisionCheck"]=  OFDictData::data(true);
    if (const Parameters::oldPressure_uniform_type* op =
            boost::get<Parameters::oldPressure_uniform_type>(&p().oldPressure) )
    {
      BC["oldPressure"] = OFDictData::toUniformField(op->value);
    }
    BC["value"]=OFDictData::toUniformField(vec3Zero());

    OFDictData::list relaxProfile;

//     if (p().relax().which()==0)
    if ( auto *rc = boost::get< Parameters::relax_constant_type>(&p().relax) )
    {
      OFDictData::list cp;
      cp.push_back(0.0);
      cp.push_back(rc->value);
      relaxProfile.push_back( cp );
    }
    else if ( const Parameters::relax_profile_type *rp = boost::get< Parameters::relax_profile_type>(&p().relax) )
    {
      for (const Parameters::relax_profile_type::values_default_type& rpi: rp->values )
      {
            OFDictData::list cp;
            cp.push_back(rpi.time);
            cp.push_back(rpi.value);
            relaxProfile.push_back(cp);
      }
    }
    BC["relax"]=  relaxProfile;

    return true;
  }
  return false;
}



}
}
