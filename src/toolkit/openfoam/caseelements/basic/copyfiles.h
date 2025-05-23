#ifndef INSIGHT_COPYFILES_H
#define INSIGHT_COPYFILES_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "copyfiles__copyFiles__Parameters_headers.h"

namespace insight {

class copyFiles
: public OpenFOAMCaseElement
{

public:
#include "copyfiles__copyFiles__Parameters.h"

/*
PARAMETERSET>>> copyFiles Parameters
inherits OpenFOAMCaseElement::Parameters

files = array [ set {
 source = path "" "File or directory which shall be copied into the case."
 target = string "constant/polyMesh" "Target location (relative to case directory)"
} ] *0 "Array of copy file operations"

createGetter
<<<PARAMETERSET
*/


public:
  declareType("copyFiles");
  copyFiles(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  void modifyFilesOnDiskBeforeDictCreation ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const override;

  static std::string category() { return "Custom"; }
};

} // namespace insight

#endif // INSIGHT_COPYFILES_H
