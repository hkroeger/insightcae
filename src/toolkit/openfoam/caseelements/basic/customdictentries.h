#ifndef INSIGHT_CUSTOMDICTENTRIES_H
#define INSIGHT_CUSTOMDICTENTRIES_H

#include "openfoam/caseelements/openfoamcaseelement.h"


namespace insight {

class customDictEntries
: public OpenFOAMCaseElement
{

public:
#include "customdictentries__customDictEntries__Parameters.h"

/*
PARAMETERSET>>> customDictEntries Parameters

entries = array [ set {
 dict = string "system/controlDict" "Path to dictionary"
 path = string "endTime" "Key or path to key. Use slashes to access subdicts. Non-existing subdicts will be created."
 value = string "1000" "Value to set (verbatim string copy will be copied)"
} ] *0 "Array of entries to set"

appendList = array [ set {
 dict = string "system/controlDict" "Path to dictionary"
 path = string "libs" "Key or path to list. Use slashes to access subdicts. Non-existing subdicts will be created."
 value = string "libSRFoption.so" "Value to append (verbatim string copy will be copied)"
} ] *0 "Array of entries to append to lists"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType("customDictEntries");
  customDictEntries(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;

  static std::string category() { return "Custom"; }
};

} // namespace insight

#endif // INSIGHT_CUSTOMDICTENTRIES_H
