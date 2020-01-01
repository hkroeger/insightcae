#ifndef INSIGHT_VECTORRESULT_H
#define INSIGHT_VECTORRESULT_H

#include "base/resultelements/numericalresult.h"


namespace insight {


class VectorResult
    : public NumericalResult<arma::mat>
{
public:
    declareType ( "VectorResult" );

    VectorResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    VectorResult ( const arma::mat& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit );
    void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const override;
    ResultElementPtr clone() const override;
};

#ifdef SWIG
%template(vectorNumericalResult) NumericalResult<arma::mat>;
#endif

} // namespace insight

#endif // INSIGHT_VECTORRESULT_H
