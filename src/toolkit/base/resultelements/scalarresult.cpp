#include "scalarresult.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {

defineType ( ScalarResult );
addToFactoryTable ( ResultElement, ScalarResult );


ScalarResult::ScalarResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : NumericalResult< double > ( shortdesc, longdesc, unit )
{
}


ScalarResult::ScalarResult ( const double& value, const string& shortDesc, const string& longDesc, const string& unit )
    : NumericalResult< double > ( value, shortDesc, longDesc, unit )
{}


void ScalarResult::writeLatexCode ( ostream& f, const std::string& , int , const boost::filesystem::path&  ) const
{
//   f.setf(ios::fixed,ios::floatfield);
//   f.precision(3);
    f << str ( format ( "%g" ) % value_ ) << unit_.toLaTeX();
}


ResultElementPtr ScalarResult::clone() const
{
    ResultElementPtr res ( new ScalarResult ( value_, shortDescription_.simpleLatex(), longDescription_.simpleLatex(), unit_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}


} // namespace insight
