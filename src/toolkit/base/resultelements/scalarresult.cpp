#include "scalarresult.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;


namespace insight {

defineType ( ScalarResult );
addToFactoryTable ( ResultElement, ScalarResult );


ScalarResult::ScalarResult (
    const std::string& shortdesc,
    const std::string& longdesc,
    const std::string& unit )
: NumericalResult< double > ( shortdesc, longdesc, unit )
{
}


ScalarResult::ScalarResult (
    const double& value,
    const string& shortDesc,
    const string& longDesc,
    const string& unit )
: NumericalResult< double > ( value, shortDesc, longDesc, unit )
{}

string ScalarResult::latexRepresentation(
    const std::string &,
    int ,
    const FileStorageInfo &) const
{
    return str ( format ( "%g" ) % value_ ) + unit_.toLaTeX();
}



std::unique_ptr<hierarchicalData::Element> ScalarResult::cloneUninitialized() const
{
    auto res =std::make_unique<ScalarResult>(
        value_,
        shortDescription_.simpleLatex(), longDescription_.simpleLatex(),
        unit_.simpleLatex() );
    res->setOrder ( order() );
    return res;
}


} // namespace insight
