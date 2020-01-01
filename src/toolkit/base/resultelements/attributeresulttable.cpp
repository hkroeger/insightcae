#include "attributeresulttable.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {


defineType ( AttributeTableResult );
addToFactoryTable ( ResultElement, AttributeTableResult );


AttributeTableResult::AttributeTableResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{
}


AttributeTableResult::AttributeTableResult
(
    AttributeNames names,
    AttributeValues values,
    const std::string& shortDesc,
    const std::string& longDesc,
    const std::string& unit
)
    : ResultElement ( shortDesc, longDesc, unit )
{
    setTableData ( names, values );
}


void AttributeTableResult::writeLatexCode ( std::ostream& f, const std::string& , int , const boost::filesystem::path& outputfilepath ) const
{
    f<<
     "\\begin{tabular}{lc}\n"
     "Attribute & Value \\\\\n"
     "\\hline\\\\";
    for ( size_t i=0; i<names_.size(); i++ ) {
        f<<names_[i]<<" & "<<values_[i]<<"\\\\"<<endl;
    }
    f<<"\\end{tabular}\n";
}


void AttributeTableResult::exportDataToFile ( const string& name, const path& outputdirectory ) const
{
    boost::filesystem::path fname ( outputdirectory/ ( name+".csv" ) );
    std::ofstream f ( fname.c_str() );

    for ( size_t i=0; i<names_.size(); i++ ) {
        f<<"\""<<names_[i]<<"\";"<<values_[i]<<endl;
    }
}


xml_node< char >* AttributeTableResult::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    for ( size_t i=0; i<names_.size(); i++ ) {
        xml_node<>* cattr = doc.allocate_node ( node_element, doc.allocate_string ( str ( format ( "attribute_%i" ) %i ).c_str() ) );
        child->append_node ( cattr );

        cattr->append_attribute ( doc.allocate_attribute
                                  (
                                      "name",
                                      doc.allocate_string ( names_[i].c_str() )
                                  ) );

        if ( const int* v = boost::get<int> ( &values_[i] ) ) {
            cattr->append_attribute ( doc.allocate_attribute (
                                          "type", doc.allocate_string ( "int" )
                                      ) );
            cattr->append_attribute ( doc.allocate_attribute (
                                          "value", doc.allocate_string ( boost::lexical_cast<std::string> ( *v ).c_str() )
                                      ) );
        } else if ( const double* v = boost::get<double> ( &values_[i] ) ) {
            cattr->append_attribute ( doc.allocate_attribute (
                                          "type", doc.allocate_string ( "double" )
                                      ) );
            cattr->append_attribute ( doc.allocate_attribute (
                                          "value", doc.allocate_string ( boost::lexical_cast<std::string> ( *v ).c_str() )
                                      ) );
        } else if ( const std::string* v = boost::get<std::string> ( &values_[i] ) ) {
            cattr->append_attribute ( doc.allocate_attribute (
                                          "type", doc.allocate_string ( "string" )
                                      ) );
            cattr->append_attribute ( doc.allocate_attribute (
                                          "value", doc.allocate_string ( v->c_str() )
                                      ) );
        }

    }

    return child;
}


ResultElementPtr AttributeTableResult::clone() const
{
    ResultElementPtr res ( new AttributeTableResult ( names_, values_,
                           shortDescription_.simpleLatex(), longDescription_.simpleLatex(), unit_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}




ResultElementPtr polynomialFitResult
(
    const arma::mat& coeffs,
    const std::string& xvarName,
    const std::string& shortDesc,
    const std::string& longDesc,
    int minorder
)
{
    std::vector<std::string> header=boost::assign::list_of ( "Term" ) ( "Coefficient" );
    AttributeTableResult::AttributeNames names;
    AttributeTableResult::AttributeValues values;

    for ( int i=coeffs.n_rows-1; i>=0; i-- ) {
        int order=minorder+i;
        if ( order==0 ) {
            names.push_back ( "$1$" );
        } else if ( order==1 ) {
            names.push_back ( "$"+xvarName+"$" );
        } else {
            names.push_back ( "$"+xvarName+"^{"+lexical_cast<string> ( order )+"}$" );
        }
        values.push_back ( coeffs ( i ) );
    }

    return ResultElementPtr
           (
               new AttributeTableResult
               (
                   names, values,
                   shortDesc, longDesc, ""
               )
           );
}


} // namespace insight
