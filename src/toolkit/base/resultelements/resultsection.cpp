#include "resultsection.h"



using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {


defineType ( ResultSection );
addToFactoryTable ( ResultElement, ResultSection );

ResultSection::ResultSection ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{}

ResultSection::ResultSection ( const std::string& sectionName, const std::string& introduction )
    : ResultElement ( "", "", "" ),
      sectionName_ ( sectionName ),
      introduction_ ( introduction )
{}

const string &ResultSection::secionName() const
{
  return sectionName_;
}

const string &ResultSection::introduction() const
{
  return introduction_;
}




void ResultSection::writeLatexCode ( ostream& f, const string& name, int level, const path& outputfilepath ) const
{
    f << latex_subsection ( level )
      << "{" << SimpleLatex(sectionName_).toLaTeX() << "}\n";
//   f << "\\label{" << cleanSymbols(name) << "}" << std::endl;  // problem with underscores: "\_" as returned by cleanSymbols is wrong here
    f << SimpleLatex(introduction_).toLaTeX() << std::endl;

    writeLatexCodeOfElements ( f, name, level, outputfilepath );
}

void ResultSection::insertLatexHeaderCode ( std::set<std::string>& hc ) const
{
    for ( const value_type& i: *this )
    {
        i.second->insertLatexHeaderCode ( hc );
    }
}

void ResultSection::exportDataToFile ( const string& name, const path& outputdirectory ) const
{
    boost::filesystem::path subdir=outputdirectory/name;

    if ( !boost::filesystem::exists ( subdir ) ) {
        boost::filesystem::create_directories ( subdir );
    }

    for ( const value_type& re: *this ) {
        re.second->exportDataToFile ( re.first, subdir );
    }
}

void ResultSection::readFromNode(const string &name, const rapidxml::xml_node<> &node)
{
  readBaseAttributesFromNode(name, node);
  sectionName_=node.first_attribute("sectionName")->value();
  introduction_=node.first_attribute("introduction")->value();
  ResultElementCollection::readElementsFromNode(node);
}

xml_node< char >* ResultSection::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    child->append_attribute ( doc.allocate_attribute
                              (
                                  "sectionName",
                                  doc.allocate_string ( sectionName_.c_str() )
                              ) );

    child->append_attribute ( doc.allocate_attribute
                              (
                                  "introduction",
                                  doc.allocate_string ( introduction_.c_str() )
                              ) );

    ResultElementCollection::appendElementsToNode ( doc, *child );

    return child;
}


std::shared_ptr< ResultElement > ResultSection::clone() const
{
    std::shared_ptr<ResultSection> res( new ResultSection ( sectionName_ ) );
    for ( const value_type& re: *this ) {
        ( *res ) [re.first] = re.second->clone();
    }
    res->setOrder ( order() );
    return std::dynamic_pointer_cast<ResultElement>( res );
}



} // namespace insight
