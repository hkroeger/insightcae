#include "cyclicacmibc.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamboundarydict.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {

defineType(CyclicACMIBC);
addToFactoryTable(BoundaryCondition, CyclicACMIBC);
addToStaticFunctionTable(BoundaryCondition, CyclicACMIBC, defaultParameters);




CyclicACMIBC::CyclicACMIBC(
    OpenFOAMCase& c, const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip )
: GGIBCBase(c, patchName, boundaryDict,
                ip.forward<Parameters>() )
{}




void CyclicACMIBC::addOptionsToBoundaryDict(OFDictData::dict &bndDict) const
{
    bndDict["nFaces"]=nFaces_;
    bndDict["startFace"]=startFace_;
    if (OFversion()>=230)
    {
      bndDict["type"]="cyclicACMI";
      bndDict["neighbourPatch"]= p().shadowPatch;
      bndDict["matchTolerance"]= 0.0001;
      bndDict["nonOverlapPatch"]=uncoupledPatchName();
    }
    else
      throw insight::Exception("cyclicACMI: unavailable feature in selected OF version!");
}




void CyclicACMIBC::addIntoFieldDictionaries(OFdicts &dictionaries) const
{
    GGIBCBase::addIntoFieldDictionaries(dictionaries);

    for (const FieldList::value_type& field: OFcase().fields())
    {
      OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
        .subDict("boundaryField").subDict(patchName_);

      if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
        MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
      else
      {
        if (OFversion()>=230)
        {
          BC["type"]=OFDictData::data("cyclicACMI");
          OFDictData::data value;
          if (get<0> ( field.second ) ==scalarField)
          {
              value="uniform 0";
          }
          else if (get<0> ( field.second ) ==vectorField)
          {
              value="uniform (0 0 0)";
          }
          else if (get<0> ( field.second ) ==symmTensorField)
          {
              value="uniform (0 0 0 0 0 0)";
          }
          BC["value"]=value;
        }
        else
          throw insight::Exception("cyclicACMI: unavailable feature in selected OF version!");
      }
    }
}




void CyclicACMIBC::modifyMeshOnDisk(const OpenFOAMCase &cm, const boost::filesystem::path &location) const
{
    // produces error, if boundaryDict entry is already present
    // if (p_.modifyMesh)
    //     modifyMesh(cm, location, {{patchName_, p_}});
}



void CyclicACMIBC::modifyMesh(
        const OpenFOAMCase &cm,
        const boost::filesystem::path &location,
        const std::vector<std::pair<
            std::string,
            Parameters> >& patchNames_Parameters )
{
    if (cm.OFversion()>=230)
    {
      // create zones
      std::vector<std::string> setSetCmds;
      for (const auto& p_p: patchNames_Parameters)
      {
          setSetCmds.push_back(
                      "faceSet "+p_p.second.zone+" new patchToFace "+p_p.first
                      );
      }
      setSet(cm, location, setSetCmds);
      cm.executeCommand(location, "setsToZones", { "-noFlipMap" } );

      // rename patches
      {
        OpenFOAMBoundaryDict bd(cm, location);
        for (const auto& p_p: patchNames_Parameters)
        {
            bd.renamePatch(p_p.first, p_p.first+"_org");
        }
        bd.write();
      }

      OFDictData::dictFile cbd;
      cbd["internalFacesOnly"]=false;
      OFDictData::dict baffles;

      for (const auto& p_p: patchNames_Parameters)
      {
          const auto& patchName = p_p.first;
          const auto& p = p_p.second;

          OFDictData::dict acmi;
          acmi["type"]="faceZone";
          acmi["zoneName"]=p.zone;

          OFDictData::dict pp;
          OFDictData::dict master, slave, master2, slave2;

          master["name"]=patchName;
//          master["type"]="patch";
          master["type"]="cyclicACMI";
          master["matchTolerance"]=0.0001;
          master["neighbourPatch"]=p.shadowPatch;
          master["nonOverlapPatch"]=uncoupledPatchName(patchName, p);
          master["transform"]="noOrdering";

          slave["name"]=patchName;
          slave["type"]="patch";

          master2["name"]=uncoupledPatchName(patchName, p);
          master2["type"]="patch";

          slave2["name"]=uncoupledPatchName(patchName, p);
          slave2["type"]="patch";

          pp["a_master"]=master;
          pp["b_slave"]=slave;
          pp["c_master2"]=master2;
          pp["d_slave2"]=slave2;
          acmi["patches"]=pp;

          baffles[patchName]=acmi;

      }

      cbd["baffles"]=baffles;

      cbd.write( location / "system" / "createBafflesDict" );

//      throw insight::Exception("STOPPED");
      cm.executeCommand(location, "createBaffles",
                        { "-overwrite" } );
    }
}

void CyclicACMIBC::modifyMesh(
    const OpenFOAMCase &cm,
    const boost::filesystem::path &location,
    const std::map<std::string, std::string> &patchNames_shadowPatchNames )
{
    std::vector<std::pair<
        std::string,
        Parameters> > pp;
    for (auto ps: patchNames_shadowPatchNames)
    {
        Parameters p;
        p.zone=ps.first;
        p.shadowPatch=ps.second;
        p.bridgeOverlap=true;
        pp.push_back({ps.first, p});
    }
    modifyMesh(cm, location, pp);
}



std::string CyclicACMIBC::uncoupledPatchName(const std::string& patchName, const Parameters& p)
{
    return patchName+"_uncoupled";
}



} // namespace insight
