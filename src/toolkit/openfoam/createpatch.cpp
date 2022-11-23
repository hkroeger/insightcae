#include "openfoam/createpatch.h"

namespace insight {


namespace createPatchOps
{




createPatchOperator::createPatchOperator(const ParameterSet& p )
: p_(p)
{}

createPatchOperator::~createPatchOperator()
{}


void createPatchOperator::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const
{
  OFDictData::dict opdict;
  opdict["name"]=p_.name;
  opdict["constructFrom"]=p_.constructFrom;
  OFDictData::list pl;
  std::copy(p_.patches.begin(), p_.patches.end(), std::back_inserter(pl));
  opdict["patches"]=pl;
  opdict["set"]=p_.setname;

  OFDictData::dict opsubdict;
  opsubdict["type"]=p_.patchtype;

  if (ofc.OFversion()<170)
  {
    opdict["dictionary"]=opsubdict;
    createPatchDict.getList("patchInfo").push_back( opdict );
  }
  else
  {
    opdict["patchInfo"]=opsubdict;
    createPatchDict.getList("patches").push_back( opdict );
  }

}





createCyclicOperator::createCyclicOperator(const ParameterSet& p )
: p_(p)
{}

void createCyclicOperator::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const
{
  std::vector<std::string> suffixes;
  if (ofc.OFversion()>=210)
  {
    suffixes.push_back("_half0");
    suffixes.push_back("_half1");
  }
  else
    suffixes.push_back("");

  for (const std::string& suf: suffixes)
  {
    OFDictData::dict opdict;
    opdict["name"]=p_.name+suf;
    opdict["constructFrom"]=p_.constructFrom;
    OFDictData::list pl;
    if (suf=="_half0" || suf=="")
    {
      pl.resize(pl.size()+p_.patches.size());
      std::copy(p_.patches.begin(), p_.patches.end(), pl.begin());
      opdict["set"]=p_.setname;
    }
    if (suf=="_half1" || suf=="")
    {
      size_t osize=pl.size();
      pl.resize(osize+p_.patches_half1.size());
      std::copy(p_.patches_half1.begin(), p_.patches_half1.end(), pl.begin()+osize);
      if (suf!="") opdict["set"]=p_.set_half1;
    }
    opdict["patches"]=pl;

    OFDictData::dict opsubdict;
    opsubdict["type"]="cyclic";
    if (suf=="_half0") opsubdict["neighbourPatch"]=p_.name+"_half1";
    if (suf=="_half1") opsubdict["neighbourPatch"]=p_.name+"_half0";

    if (ofc.OFversion()>=210)
    {
      opdict["patchInfo"]=opsubdict;
      createPatchDict.getList("patches").push_back( opdict );
    }
    else
    {
      opdict["dictionary"]=opsubdict;
      createPatchDict.getList("patchInfo").push_back( opdict );
    }
  }

}





}





void createPatch(
        const OpenFOAMCase& ofc,
        const boost::filesystem::path& location,
        const std::vector<createPatchOps::createPatchOperatorPtr>& ops,
        bool overwrite
        )
{
  using namespace createPatchOps;

  OFDictData::dictFile createPatchDict;

  createPatchDict["matchTolerance"] = 1e-3;
  createPatchDict["pointSync"] = false;

  if (ofc.OFversion()<170)
    createPatchDict.getList("patchInfo");
  else
    createPatchDict.getList("patches");

  for ( const auto op: ops)
  {
    op->addIntoDictionary(ofc, createPatchDict);
  }

  // then write to file
  createPatchDict.write( location / "system" / "createPatchDict" );

  std::vector<std::string> opts;
  if (overwrite) opts.push_back("-overwrite");

  ofc.executeCommand(location, "createPatch", opts);
}




} // namespace insight
