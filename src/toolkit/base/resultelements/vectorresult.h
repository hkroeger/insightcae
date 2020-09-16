#ifndef INSIGHT_VECTORRESULT_H
#define INSIGHT_VECTORRESULT_H

#include "base/resultelements/numericalresult.h"


namespace insight {

#ifdef SWIG
%template(vectorNumericalResult) NumericalResult<arma::mat>;
#endif
typedef NumericalResult<arma::mat> vectorNumericalResult;

class VectorResult
    : public vectorNumericalResult
{
public:
    declareType ( "VectorResult" );

    VectorResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    VectorResult ( const arma::mat& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit );
    void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const override;
    ResultElementPtr clone() const override;
};


} // namespace insight



#endif // INSIGHT_VECTORRESULT_H
