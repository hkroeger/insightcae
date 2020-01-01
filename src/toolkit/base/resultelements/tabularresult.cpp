#include "tabularresult.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {

defineType ( TabularResult );
addToFactoryTable ( ResultElement, TabularResult );


TabularResult::TabularResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{
}


TabularResult::TabularResult
(
    const std::vector<std::string>& headings,
    const Table& rows,
    const std::string& shortDesc,
    const std::string& longDesc,
    const std::string& unit
)
    : ResultElement ( shortDesc, longDesc, unit )
{
    setTableData ( headings, rows );
}


TabularResult::TabularResult
(
    const std::vector<std::string>& headings,
    const arma::mat& rows,
    const std::string& shortDesc,
    const std::string& longDesc,
    const std::string& unit
)
    : ResultElement ( shortDesc, longDesc, unit )
{
    Table t;
    for ( arma::uword i=0; i<rows.n_rows; i++ ) {
        Row r;
        for ( arma::uword j=0; j<rows.n_cols; j++ ) {
            r.push_back ( rows ( i,j ) );
        }
        t.push_back ( r );
    }
    setTableData ( headings, t );
}


void TabularResult::setCellByName ( TabularResult::Row& r, const string& colname, double value )
{
    std::vector<std::string>::const_iterator ii=std::find ( headings_.begin(), headings_.end(), colname );
    if ( ii==headings_.end() ) {
        std::ostringstream msg;
        msg<<"Tried to write into column "+colname+" but this does not exist! Existing columns are:"<<endl;
        for ( const std::string& n: headings_ ) {
            msg<<n<<endl;
        }
        insight::Exception ( msg.str() );
    }
    int i= ii - headings_.begin();
    r[i]=value;
}


arma::mat TabularResult::getColByName ( const string& colname ) const
{
    std::vector<std::string>::const_iterator ii=std::find ( headings_.begin(), headings_.end(), colname );
    if ( ii==headings_.end() ) {
        std::ostringstream msg;
        msg<<"Tried to get column "+colname+" but this does not exist! Existing columns are:"<<endl;
        for ( const std::string& n: headings_ ) {
            msg<<n<<endl;
        }
        insight::Exception ( msg.str() );
    }
    int i= ii - headings_.begin();
    return toMat().col ( i );
}


arma::mat TabularResult::toMat() const
{
    arma::mat res;
    res.resize ( rows_.size(), rows_[0].size() );
    int i=0;
    for ( const std::vector<double>& row: rows_ ) {
        int j=0;
        for ( double v: row ) {
//             cout<<"res("<<i<<","<<j<<")="<<v<<endl;
            res ( i, j++ ) =v;
        }
        i++;
    }
    return res;
}


void TabularResult::writeGnuplotData ( std::ostream& f ) const
{
    f<<"#";
    for ( const std::string& head: headings_ ) {
        f<<" \""<<head<<"\"";
    }
    f<<std::endl;

    for ( const std::vector<double>& row: rows_ ) {
        for ( double v: row ) {
            f<<" "<<v;
        }
        f<<std::endl;
    }

}


ResultElementPtr TabularResult::clone() const
{
    ResultElementPtr res ( new TabularResult ( headings_, rows_, shortDescription_.simpleLatex(), longDescription_.simpleLatex(), unit_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}


void TabularResult::writeLatexHeaderCode ( ostream& f ) const
{
    insight::ResultElement::writeLatexHeaderCode ( f );
    f<<"\\usepackage{longtable}\n";
    f<<"\\usepackage{placeins}\n";
}


void TabularResult::writeLatexCode ( std::ostream& f, const std::string& , int , const boost::filesystem::path&  ) const
{
  std::vector<std::vector<int> > colsets;
  int i=0;
  std::vector<int> ccolset;
  for(size_t c=0; c<headings_.size(); c++)
  {
    ccolset.push_back(c);
    i++;
    if (i>5) { colsets.push_back(ccolset); ccolset.clear(); i=0; }
  }
  if (ccolset.size()>0) colsets.push_back(ccolset);

  for (const std::vector<int>& cols: colsets)
  {
    f<<
     "\\begin{longtable}{";
    for (int c: cols) {
        f<<"c";
    }
    f<<"}\n";

    for (size_t i=0; i<cols.size(); i++) {
        if ( i!=0 ) {
            f<<" & ";
        }
        f<<headings_[cols[i]];
    }
    f<<
     "\\\\\n"
     "\\hline\n"
     "\\endfirsthead\n"
     "\\endhead\n";
    for ( TabularResult::Table::const_iterator i=rows_.begin(); i!=rows_.end(); i++ ) {
        if ( i!=rows_.begin() ) {
            f<<"\\\\\n";
        }
//        for ( std::vector<double>::const_iterator j=i->begin(); j!=i->end(); j++ ) {
        for (size_t j=0; j< cols.size(); j++) {
            if ( j!=0 ) {
                f<<" & ";
            }
            if ( !std::isnan ( (*i)[cols[j]] ) ) {
                f<<(*i)[cols[j]];
            }
        }
    }
    f<<
     "\\end{longtable}\n\n"
//     "\\newpage\n"  // page break algorithm fails after too short "longtable"
        ;
  }
}


xml_node< char >* TabularResult::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    xml_node<>* heads = doc.allocate_node ( node_element, doc.allocate_string ( "headings" ) );
    child->append_node ( heads );
    for ( size_t i=0; i<headings_.size(); i++ ) {
        xml_node<>* chead = doc.allocate_node ( node_element, doc.allocate_string ( str ( format ( "header_%i" ) %i ).c_str() ) );
        heads->append_node ( chead );

        chead->append_attribute ( doc.allocate_attribute
                                  (
                                      "title",
                                      doc.allocate_string ( headings_[i].c_str() )
                                  ) );
    }

    xml_node<>* values = doc.allocate_node ( node_element, doc.allocate_string ( "values" ) );
    child->append_node ( values );
    writeMatToXMLNode ( toMat(), doc, *values );

    return child;
}


void TabularResult::exportDataToFile ( const string& name, const path& outputdirectory ) const
{
    boost::filesystem::path fname ( outputdirectory/ ( name+".csv" ) );
    std::ofstream f ( fname.c_str() );

    std::string sep="";
    for ( const std::string& h: headings_ ) {
        f<<sep<<"\""<<h<<"\"";
        sep=";";
    }
    f<<endl;

    for ( const Row& r: rows_ ) {
        sep="";
        for ( const double& v: r ) {
            f<<sep<<v;
            sep=";";
        }
        f<<endl;
    }
}



} // namespace insight
