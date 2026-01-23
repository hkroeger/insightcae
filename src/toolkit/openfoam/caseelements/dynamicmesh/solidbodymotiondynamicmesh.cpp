#include "solidbodymotiondynamicmesh.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


solidBodyMotionFunction::solidBodyMotionFunction(const Parameters& p)
    : mp_(p)
{}

void solidBodyMotionFunction::addIntoDictionary(OFDictData::dict& sbc) const
{
    if ( auto* rp = boost::get<Parameters::motion_rotation_type>(&mp_.motion) )
    {
        sbc["solidBodyMotionFunction"]="rotatingMotion";
        OFDictData::dict rmc;
        rmc["origin"]=OFDictData::vector3(rp->origin);
        rmc["axis"]=OFDictData::vector3(rp->axis);
        rmc["omega"]=2.*M_PI*rp->rpm/60.;
        sbc["rotatingMotionCoeffs"]=rmc;
    }
    else if ( auto* tr = boost::get<Parameters::motion_translation_type>(&mp_.motion) )
    {
        sbc["solidBodyMotionFunction"]="linearMotion";
        OFDictData::dict rmc;
        rmc["velocity"]=OFDictData::vector3(tr->velocity);
        sbc["linearMotionCoeffs"]=rmc;
    }
    else if ( auto* ro = boost::get<Parameters::motion_oscillatingRotating_type>(&mp_.motion) )
    {
        sbc["solidBodyMotionFunction"]="oscillatingRotatingMotion";
        OFDictData::dict rmc;
        rmc["origin"]=OFDictData::vector3(ro->origin);
        rmc["omega"]=ro->omega;
        rmc["amplitude"]=OFDictData::vector3(ro->amplitude);
        sbc["oscillatingRotatingMotionCoeffs"]=rmc;
    }
    else throw insight::UnhandledSelection();
}

defineType(solidBodyMotionDynamicMesh);
addToOpenFOAMCaseElementFactoryTable(solidBodyMotionDynamicMesh);



solidBodyMotionDynamicMesh::solidBodyMotionDynamicMesh(
    OpenFOAMCase& c, ParameterSetInput ip )
: dynamicMesh(c, ip.forward<Parameters>()),
    solidBodyMotionFunction(p())
{}


void solidBodyMotionDynamicMesh::addIntoDictionaries(OFdicts& dictionaries) const
{
    OFDictData::dict& dynamicMeshDict
      = dictionaries.lookupDict("constant/dynamicMeshDict");

    dynamicMeshDict["dynamicFvMesh"]="dynamicMotionSolverFvMesh";
    dynamicMeshDict["solver"]="solidBody";
    OFDictData::dict &sbc=dynamicMeshDict.subDict("solidBodyCoeffs");
    sbc["cellZone"]=p().zonename;
    solidBodyMotionFunction::addIntoDictionary(sbc);
}



} // namespace insight
