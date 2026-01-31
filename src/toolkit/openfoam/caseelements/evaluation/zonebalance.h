#ifndef ZONEBALANCE_H
#define ZONEBALANCE_H

#include "boost/regex/v4/regex.hpp"
#include "openfoam/caseelements/analysiscaseelements.h"
#include "openfoam/outputsectionreader.h"

#include "zonebalance__zoneBalance__Parameters_headers.h"


namespace insight {


namespace OutputSectionReaders {

class zoneBalance : public OutputSectionReader
{
    std::string label_;
    std::string varDesc_;
    int ncells_;
    double Vol_;
    double balance_;

    zoneBalance(const boost::smatch& groups);

public:
    declareType("zoneBalance");

    static std::unique_ptr<OutputSectionReader> createIfMatches(
        const std::string& line );

    void addProgressVariables(std::map<std::string, double>& progVars) const override;
};

}



class zoneBalance
    : public functionObject
{
public:
#include "zonebalance__zoneBalance__Parameters.h"
/*
PARAMETERSET>>> zoneBalance Parameters
inherits functionObject::Parameters

description
"This case element add an integration function object to the OpenFOAM case"

cellSelection = selectablesubset {{
 all set {}
 cellSet set {
  name = string "" "Name of the cell set"
 }
 threshold set {
  thresholdScalarFieldName = string "" "name of the selection scalar field"
  lowerThreshold = double -1e15 "lower bound of range with eligible value"
  upperThreshold = double 1e15 "upper bound of range with eligible value"
 }
 highestValueVolumeFraction set {
  thresholdScalarFieldName = string "" "name of the selection scalar field"
  volumeFraction = double 1e-3 "fraction of volume with the highest values"
 }
}} all ""

factorFields = array [
  string "" "name of field which shall be multiplied with flux field"
] *0 "cumulative multipliers for flux field"

createGetter
<<<PARAMETERSET
*/

public:
    declareType("zoneBalance");
    zoneBalance(OpenFOAMCase &c, ParameterSetInput ip = Parameters());
    OFDictData::dict functionObjectDict() const override;
    std::set<std::string> requiredLibraries() const override;

    static arma::mat readBalance
        (
            const OpenFOAMCase& c,
            const boost::filesystem::path& location,
            const std::string& foName
            );
};

} // namespace insight

#endif
