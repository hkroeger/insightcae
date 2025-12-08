#include "vectorresult.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {

defineType ( VectorResult );
addToFactoryTable ( ResultElement, VectorResult );


VectorResult::VectorResult (
    const std::string& shortdesc,
    const std::string& longdesc,
    const std::string& unit )
    : NumericalResult< arma::mat > ( shortdesc, longdesc, unit )
{
}


VectorResult::VectorResult (
    const arma::mat& value,
    const string& shortDesc,
    const string& longDesc,
    const string& unit )
    : NumericalResult< arma::mat > ( value, shortDesc, longDesc, unit )
{}


std::string
VectorResult::latexRepresentation(
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
    return
        str ( format ( "(%g %g %g)" ) % value_(0) % value_(1) % value_(2) )
        + unit_.toLaTeX();
}


std::unique_ptr<hierarchicalData::Element> VectorResult::clone() const
{
    auto res = std::make_unique<VectorResult> (
        value_, shortDescription_.simpleLatex(),
        longDescription_.simpleLatex(), unit_.simpleLatex() );
    res->setOrder ( order() );
    return res;
}


} // namespace insight
