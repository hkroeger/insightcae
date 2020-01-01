#include "vectorresult.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {

defineType ( VectorResult );
addToFactoryTable ( ResultElement, VectorResult );


VectorResult::VectorResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : NumericalResult< arma::mat > ( shortdesc, longdesc, unit )
{
}


VectorResult::VectorResult ( const arma::mat& value, const string& shortDesc, const string& longDesc, const string& unit )
    : NumericalResult< arma::mat > ( value, shortDesc, longDesc, unit )
{}


void VectorResult::writeLatexCode ( ostream& f, const std::string& , int , const boost::filesystem::path&  ) const
{
//   f.setf(ios::fixed,ios::floatfield);
//   f.precision(3);
    f << str ( format ( "(%g %g %g)" ) % value_(0) % value_(1) % value_(2) ) << unit_.toLaTeX();
}


ResultElementPtr VectorResult::clone() const
{
    ResultElementPtr res ( new VectorResult ( value_, shortDescription_.simpleLatex(), longDescription_.simpleLatex(), unit_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}


} // namespace insight
