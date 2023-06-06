#include "solidbodymotiondynamicmesh.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {

defineType(solidBodyMotionDynamicMesh);
addToOpenFOAMCaseElementFactoryTable(solidBodyMotionDynamicMesh);



solidBodyMotionDynamicMesh::solidBodyMotionDynamicMesh( OpenFOAMCase& c, const ParameterSet& ps )
: dynamicMesh(c, ps),
  ps_(ps)
{
}


void solidBodyMotionDynamicMesh::addIntoDictionaries(OFdicts& dictionaries) const
{
    Parameters p(ps_);

    OFDictData::dict& dynamicMeshDict
      = dictionaries.lookupDict("constant/dynamicMeshDict");

    dynamicMeshDict["dynamicFvMesh"]="dynamicMotionSolverFvMesh";
    dynamicMeshDict["solver"]="solidBody";
    OFDictData::dict sbc;

    sbc["cellZone"]=p.zonename;

    if ( auto* rp = boost::get<Parameters::motion_rotation_type>(&p.motion) )
    {
        sbc["solidBodyMotionFunction"]="rotatingMotion";
        OFDictData::dict rmc;
        rmc["origin"]=OFDictData::vector3(rp->origin);
        rmc["axis"]=OFDictData::vector3(rp->axis);
        rmc["omega"]=2.*M_PI*rp->rpm/60.;
        sbc["rotatingMotionCoeffs"]=rmc;
    }
    else if ( auto* tr = boost::get<Parameters::motion_translation_type>(&p.motion) )
    {
        sbc["solidBodyMotionFunction"]="linearMotion";
        OFDictData::dict rmc;
        rmc["velocity"]=OFDictData::vector3(tr->velocity);
        sbc["linearMotionCoeffs"]=rmc;
    }
    else if ( auto* ro = boost::get<Parameters::motion_oscillatingRotating_type>(&p.motion) )
    {
        sbc["solidBodyMotionFunction"]="oscillatingRotatingMotion";
        OFDictData::dict rmc;
        rmc["origin"]=OFDictData::vector3(ro->origin);
        rmc["omega"]=ro->omega;
        rmc["amplitude"]=OFDictData::vector3(ro->amplitude);
        sbc["oscillatingRotatingMotionCoeffs"]=rmc;
    }
    else
      throw insight::Exception("Internal error: Unhandled selection!");

    dynamicMeshDict["solidBodyCoeffs"]=sbc;
}



} // namespace insight
