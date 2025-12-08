#ifndef INSIGHT_SCALARRESULT_H
#define INSIGHT_SCALARRESULT_H

#include "base/resultelements/numericalresult.h"


namespace insight {

#ifdef SWIG
%template(doubleNumericalResult) NumericalResult<double>;
#endif
typedef NumericalResult<double> doubleNumericalResult;

class ScalarResult
    : public doubleNumericalResult
{
public:
    declareType ( "ScalarResult" );

    ScalarResult (
        const std::string& shortdesc,
        const std::string& longdesc,
        const std::string& unit );

    ScalarResult (
        const double& value,
        const std::string& shortDesc,
        const std::string& longDesc,
        const std::string& unit );

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    std::unique_ptr<hierarchicalData::Element> clone() const override;
};


} // namespace insight


#endif // INSIGHT_SCALARRESULT_H
