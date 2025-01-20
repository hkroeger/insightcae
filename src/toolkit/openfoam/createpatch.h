#ifndef INSIGHT_CREATEPATCH_H
#define INSIGHT_CREATEPATCH_H


#include "openfoam/openfoamcase.h"
#include "createpatch__createPatchOperator__Parameters_headers.h"



namespace insight
{



namespace createPatchOps
{




class createPatchOperator
{
public:
#include "createpatch__createPatchOperator__Parameters.h"
/*
PARAMETERSET>>> createPatchOperator Parameters

name = string "newpatch" ""
constructFrom = string "patches" ""
patchtype = string "patch" ""
patches = array [ string "" "Patch name" ] *0 "Name of patches"
setname = string "set" ""

createGetters
<<<PARAMETERSET
*/

public:
  createPatchOperator(ParameterSetInput ip = Parameters() );
  virtual ~createPatchOperator();

  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const;
};


typedef std::shared_ptr<createPatchOperator> createPatchOperatorPtr;




/**
 * Creates a cyclic patch or cyclic patch pair (depending on OF version)
 * from two other patches
 */
class createCyclicOperator
: public createPatchOperator
{
public:
#include "createpatch__createCyclicOperator__Parameters.h"
/*
PARAMETERSET>>> createCyclicOperator Parameters
inherits createPatchOperator::Parameters

patches_half1 = array [ string "" "" ]*0 ""
set_half1 = string "set_half1" ""

createGetters
<<<PARAMETERSET
*/

public:
  createCyclicOperator(ParameterSetInput ip = Parameters() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const;
};




}




void createPatch(
        const OpenFOAMCase& ofc,
        const boost::filesystem::path& location,
        const std::vector<
#ifndef SWIG
        createPatchOps::
#endif
        createPatchOperatorPtr>& ops,
        bool overwrite=true
        );



} // namespace insight

#endif // INSIGHT_CREATEPATCH_H
