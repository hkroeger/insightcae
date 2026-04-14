#include "attributeresulttable.h"
#include "base/hierarchicalelement.h"
#include "base/rapidxml.h"
#include <boost/lexical_cast.hpp>
#include <sstream>


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;


namespace insight {


defineType ( AttributeTableResult );
addToFactoryTable ( ResultElement, AttributeTableResult );




SimpleLatex AttributeTableResult::defaultLabelColumnTitle("Attribute");
SimpleLatex AttributeTableResult::defaultValueColumnTitle("Value");




AttributeTableResult::AttributeTableResult(
        const std::string& shortdesc,
        const std::string& longdesc,
        const std::string& unit,
        const SimpleLatex& labelColumnTitle,
        const SimpleLatex& valueColumnTitle )
    : ResultElement ( shortdesc, longdesc, unit ),
      labelColumnTitle_(labelColumnTitle),
      valueColumnTitle_(valueColumnTitle)
{
}


AttributeTableResult::AttributeTableResult(
        AttributeNames names,
        AttributeValues values,
        const std::string& shortDesc,
        const std::string& longDesc,
        const std::string& unit,
        const SimpleLatex& labelColumnTitle,
        const SimpleLatex& valueColumnTitle
)
    : ResultElement ( shortDesc, longDesc, unit ),
      labelColumnTitle_(labelColumnTitle),
      valueColumnTitle_(valueColumnTitle)
{
    setTableData ( names, values );
}



const SimpleLatex& AttributeTableResult::labelColumnTitle() const
{
    return labelColumnTitle_;
}



const SimpleLatex& AttributeTableResult::valueColumnTitle() const
{
    return valueColumnTitle_;
}

string AttributeTableResult::value(int i) const
{
    if (auto* s=boost::get<std::string>(&values_[i]))
    {
        return *s;
    }
    else if (auto* d=boost::get<double>(&values_[i]))
    {
        return insight::toString(*d);
    }
    else if (auto* d=boost::get<int>(&values_[i]))
    {
        return insight::toString(*d);
    }

    return std::string();
}



std::string AttributeTableResult::latexRepresentation(
    const std::string&,
    int,
    const FileStorageInfo& ) const
{
    std::ostringstream f;
    f<< "\\begin{longtable}{lc}\n"
     << "\\hline\n"
     << labelColumnTitle_.toLaTeX()
     << " & "
     << valueColumnTitle_.toLaTeX()
     <<" \\\\\n"
     "\\hline\n"
     "\\endfirsthead\n"
     "\\endhead\n";
    for ( size_t i=0; i<names_.size(); i++ )
    {
        f<<names_[i].toLaTeX()<<" & "<<value(i)<<"\\\\"<<endl;
    }
    f<<"\\hline\n";
    f<<"\\end{longtable}\n";
    return f.str();
}




void AttributeTableResult::exportDataToFile (
    const string& name,
    const boost::filesystem::path& outputdirectory ) const
{
    boost::filesystem::path fname ( outputdirectory/ ( name+".csv" ) );
    std::ofstream f ( fname.c_str() );

    for ( size_t i=0; i<names_.size(); i++ ) {
        f<<"\""<<names_[i].toPlainText()<<"\"\t"<<value(i)<<endl;
    }
}




const rapidxml::xml_node<>*
AttributeTableResult::readFromNode(
    const string &name,
    const rapidxml::xml_node<> &parentNode )
{
  auto *child = ResultElement::readFromNode(name, parentNode);
  if (auto *vct = child->first_attribute("valueColumnTitle"))
  {
      valueColumnTitle_=SimpleLatex(vct->value());
  }
  if (auto *lct = child->first_attribute("labelColumnTitle"))
  {
      labelColumnTitle_=SimpleLatex(lct->value());
  }
  for (xml_node<> *e = child->first_node(); e; e = e->next_sibling())
  {
    names_.push_back(SimpleLatex(e->first_attribute("name")->value()));
    std::string typ(e->first_attribute("type")->value());
    std::string value(e->first_attribute("value")->value());
    if (typ=="int")
      values_.push_back(boost::lexical_cast<int>(value));
    else if (typ=="double")
      values_.push_back(boost::lexical_cast<double>(value));
    else if (typ=="string")
      values_.push_back(value);
  }
  return child;
}

int AttributeTableResult::nChildren() const
{
    return 0;
}

bool AttributeTableResult::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const AttributeTableResult*>(&op))
    {
        if (labelColumnTitle_!=oa->labelColumnTitle_)
            return false;
        if (valueColumnTitle_!=oa->valueColumnTitle_)
            return false;

        if (names_.size()!=oa->names_.size())
            return false;
        if (values_.size()!=oa->values_.size())
            return false;

        {
            auto i=names_.begin();
            auto j=oa->names_.begin();

            while (i!=names_.end())
            {
                if (*i!=*j)
                    return false;

                ++i; ++j;
            }
        }

        {
            auto i=values_.begin();
            auto j=oa->values_.begin();

            while (i!=values_.end())
            {
                if (*i!=*j)
                    return false;

                ++i; ++j;
            }
        }

        return true;
    }
    else
        return false;
}


xml_node< char >* AttributeTableResult::appendToNode (
    const string& name,
    xml_document< char >& doc,
    xml_node< char >& node,
    const OutputProperties& outProps ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node, outProps );

    if (labelColumnTitle()!=defaultLabelColumnTitle)
    {
        appendAttribute(doc, *child, "labelColumnTitle", labelColumnTitle().simpleLatex());
    }
    if (valueColumnTitle()!=defaultValueColumnTitle)
    {
        appendAttribute(doc, *child, "valueColumnTitle", valueColumnTitle().simpleLatex());
    }

    for ( size_t i=0; i<names_.size(); i++ ) {

        auto cattr = appendNode(doc, *child, str ( format ( "attribute_%i" ) %i ));

        appendAttribute(doc, cattr, "name", names_[i].simpleLatex());

        if ( const int* v = boost::get<int> ( &values_[i] ) )
        {
            appendAttribute(doc, cattr, "type", "int" );
            appendAttribute(doc, cattr, "value", *v );
        }
        else if ( const double* v = boost::get<double> ( &values_[i] ) )
        {
            appendAttribute(doc, cattr, "type", "double");
            appendAttribute(doc, cattr, "value", *v );
        }
        else if ( const std::string* v = boost::get<std::string> ( &values_[i] ) )
        {
            appendAttribute(doc, cattr, "type", "string" );
            appendAttribute(doc, cattr, "value", *v);
        }

    }

    return child;
}


std::unique_ptr<hierarchicalData::Element>
AttributeTableResult::cloneUninitialized() const
{
    auto cl = std::make_unique<AttributeTableResult>(
        names_, values_,
        shortDescription_.simpleLatex(), longDescription_.simpleLatex(), unit_.simpleLatex(),
        labelColumnTitle_, valueColumnTitle_ );
    cl->setOrder ( order() );
    return cl;
}




std::unique_ptr<ResultElement> polynomialFitResult
(
    const arma::mat& coeffs,
    const std::string& xvarName,
    const std::string& shortDesc,
    const std::string& longDesc,
    int minorder
)
{
    std::vector<std::string> header{ "Term", "Coefficient" };
    AttributeTableResult::AttributeNames names;
    AttributeTableResult::AttributeValues values;

    for ( int i=coeffs.n_rows-1; i>=0; i-- ) {
        int order=minorder+i;
        if ( order==0 ) {
            names.push_back ( {"$1$"} );
        } else if ( order==1 ) {
            names.push_back ( "$"+xvarName+"$" );
        } else {
            names.push_back ( "$"+xvarName+"^{"+toString( order )+"}$" );
        }
        values.push_back ( coeffs ( i ) );
    }

    return std::make_unique<AttributeTableResult>
               (
                   names, values,
                   shortDesc, longDesc, ""
               );
}


} // namespace insight
