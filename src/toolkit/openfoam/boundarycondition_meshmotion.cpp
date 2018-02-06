#include "openfoam/boundarycondition_meshmotion.h"


#include <string>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;


namespace insight
{
namespace MeshMotionBC
{




defineType(MeshMotionBC);
defineDynamicClass(MeshMotionBC);

MeshMotionBC::~MeshMotionBC()
{
}


void MeshMotionBC::addIntoDictionaries(OFdicts&) const
{}




defineType(NoMeshMotion);
addToFactoryTable(MeshMotionBC, NoMeshMotion);
addToStaticFunctionTable(MeshMotionBC, NoMeshMotion, defaultParameters);

NoMeshMotion::NoMeshMotion(const ParameterSet& ps)
{}

bool NoMeshMotion::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
    if
    (
      ((fieldname=="displacement")||(fieldname == "motionU"))
      &&
      (get<0>(fieldinfo)==vectorField)
    )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform (0 0 0)");
      return true;
    }
    else
      return false;
}

NoMeshMotion noMeshMotion;




defineType(CAFSIBC);
addToFactoryTable(MeshMotionBC, CAFSIBC);
addToStaticFunctionTable(MeshMotionBC, CAFSIBC, defaultParameters);

CAFSIBC::CAFSIBC(const ParameterSet& ps)
: p_(ps)
{
}


void CAFSIBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libFEMDisplacementBC.so\"") );
}

bool CAFSIBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "motionU")
  {
    BC["prescribeMotionVelocity"] = OFDictData::data(true);
  }
  if ( (fieldname == "pointDisplacement") || (fieldname == "motionU") )
  {
    BC["type"]= OFDictData::data("FEMDisplacement");
    BC["FEMCaseDir"]=  OFDictData::data(std::string("\"")+p_.FEMScratchDir.c_str()+"\"");
    BC["pressureScale"]=  OFDictData::data(p_.pressureScale);
    BC["minPressure"]=  OFDictData::data(p_.clipPressure);
    BC["nSmoothIter"]=  OFDictData::data(4);
    BC["wallCollisionCheck"]=  OFDictData::data(true);
    if (const Parameters::oldPressure_uniform_type* op = boost::get<Parameters::oldPressure_uniform_type>(&p_.oldPressure) )
    {
      std::ostringstream oss;
      oss<<"uniform "<<op->value;
      BC["oldPressure"] = OFDictData::data(oss.str());
    }
    BC["value"]=OFDictData::data("uniform (0 0 0)");

    OFDictData::list relaxProfile;

//     if (p_.relax().which()==0)
    if ( const Parameters::relax_constant_type *rc = boost::get< Parameters::relax_constant_type>(&p_.relax) )
    {
      OFDictData::list cp;
      cp.push_back(0.0);
      cp.push_back( boost::get<double>(rc->value) );
      relaxProfile.push_back( cp );
    }
    else if ( const Parameters::relax_profile_type *rp = boost::get< Parameters::relax_profile_type>(&p_.relax) )
    {
      BOOST_FOREACH(const Parameters::relax_profile_type::values_default_type& rpi,  rp->values )
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
