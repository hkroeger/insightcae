#include "tabularresult.h"

#include "base/rapidxml.h"
#include <iterator>


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {

defineType ( TabularResult );
addToFactoryTable ( ResultElement, TabularResult );


TabularResult::TabularResult (
    const std::string& shortdesc,
    const std::string& longdesc,
    const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{
}


TabularResult::TabularResult
    (
        const Headings &headings,
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
    const Headings& headings,
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
    auto ii=std::find_if ( headings_.begin(), headings_.end(), [&](const SimpleLatex& h){ return h.simpleLatex()==colname; } );
    if ( ii==headings_.end() ) {
        std::ostringstream msg;
        msg<<"Tried to write into column "+colname+" but this does not exist! Existing columns are:"<<endl;
        for ( auto& n: headings_ ) {
            msg<<n.simpleLatex()<<endl;
        }
        insight::Exception ( msg.str() );
    }
    int i= ii - headings_.begin();
    r[i]=value;
}


arma::mat TabularResult::getColByName ( const string& colname ) const
{
    auto ii=std::find_if ( headings_.begin(), headings_.end(), [&](const SimpleLatex& h){ return h.simpleLatex()==colname; } );
    if ( ii==headings_.end() ) {
        std::ostringstream msg;
        msg<<"Tried to get column "+colname+" but this does not exist! Existing columns are:"<<endl;
        for ( auto& n: headings_ ) {
            msg<<n.simpleLatex()<<endl;
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
    for ( auto& head: headings_ ) {
        f<<" \""<<head.toPlainText()<<"\"";
    }
    f<<std::endl;

    for ( const std::vector<double>& row: rows_ ) {
        for ( double v: row ) {
            f<<" "<<v;
        }
        f<<std::endl;
    }

}


std::unique_ptr<hierarchicalData::Element> TabularResult::clone() const
{
    auto res=std::make_unique<TabularResult>(
        headings_, rows_,
        shortDescription_.simpleLatex(), longDescription_.simpleLatex(),
        unit_.simpleLatex() );
    res->setOrder ( order() );
    return res;
}


void TabularResult::insertLatexHeaderCode ( std::set<std::string>& hc ) const
{
    insight::ResultElement::insertLatexHeaderCode ( hc );
    hc.insert("\\usepackage{longtable}");
    hc.insert("\\usepackage{placeins}");
}


std::string TabularResult::latexRepresentation(
    const std::string&,
    int,
    const FileStorageInfo& ) const
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

  std::ostringstream f;
  for (const std::vector<int>& cols: colsets)
  {
    f<<
     "\\begin{longtable}{";
    for (int c=0; c<cols.size(); ++c) {
        f<<"c";
    }
    f<<"}\n";

    for (size_t i=0; i<cols.size(); i++) {
        if ( i!=0 ) {
            f<<" & ";
        }
        f<<headings_[cols[i]].toLaTeX();
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
  return f.str();
}




xml_node< char >* TabularResult::appendToNode (
    const string& name,
    xml_document< char >& doc,
    xml_node< char >& node,
    const OutputProperties& outProps ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node, outProps );

    xml_node<>* heads = doc.allocate_node ( node_element, doc.allocate_string ( "headings" ) );
    child->append_node ( heads );
    for ( size_t i=0; i<headings_.size(); i++ ) {
        xml_node<>* chead = doc.allocate_node (
              node_element,
              doc.allocate_string ( str ( format ( "header_%i" ) %i ).c_str() ) );

        heads->append_node ( chead );

        appendAttribute(doc, *chead, "title", headings_[i].simpleLatex() );
    }

    xml_node<>* values = doc.allocate_node ( node_element, doc.allocate_string ( "values" ) );
    child->append_node ( values );
    writeMatToXMLNode ( toMat(), doc, *values );

    return child;
}





const rapidxml::xml_node<>*
TabularResult::readFromNode(
    const string &name,
    const rapidxml::xml_node<> &parentNode )
{
  auto &node=*ResultElement::readFromNode(name, parentNode);

  auto* heads = node.first_node("headings");
  for (xml_node<> *e = heads->first_node(); e; e = e->next_sibling())
  {
    headings_.push_back(SimpleLatex(getMandatoryAttribute(*e, "title")));
  }

  auto* vals = node.first_node("values");
  std::istringstream iss(vals->value());
  arma::mat mat;
  mat.load(iss, arma::raw_ascii);
  for (arma::uword i=0; i<mat.n_rows; i++)
  {
    Row r;
    for (arma::uword j=0; j<mat.n_cols; j++)
    {
      r.push_back(mat(i,j));
    }
    rows_.push_back(r);
  }

  return &node;
}




bool TabularResult::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const TabularResult*>(&op))
    {
        if (headings_.size()!=oa->headings_.size())
            return false;
        if (rows_.size()!=oa->rows_.size())
            return false;

        {
            auto i=headings_.begin();
            auto j=oa->headings_.begin();

            while (i!=headings_.end())
            {
                if (*i!=*j)
                    return false;

                ++i; ++j;
            }
        }

        {
            auto i=rows_.begin();
            auto j=oa->rows_.begin();

            while (i!=rows_.end())
            {
                if (i->size()!=j->size())
                    return false;

                {
                    auto l=i->begin();
                    auto m=j->begin();

                    while (l!=i->end())
                    {
                        if (*l!=*m)
                            return false;

                        ++l; ++m;
                    }
                }
                ++i; ++j;
            }
        }

        return true;
    }
    else
        return false;
}


int TabularResult::nChildren() const
{
    return 0;
}

void TabularResult::exportDataToFile (
    const string& name,
    const boost::filesystem::path& outputdirectory ) const
{
    boost::filesystem::path fname ( outputdirectory/ ( name+".csv" ) );
    std::ofstream f ( fname.c_str() );

    std::string sep="";
    for ( auto& h: headings_ ) {
        f<<sep<<"\""<<h.toPlainText()<<"\"";
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
