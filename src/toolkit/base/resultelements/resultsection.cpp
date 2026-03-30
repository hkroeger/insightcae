#include "resultsection.h"
#include "base/rapidxml.h"



using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;


namespace insight {


defineType ( ResultSection );
addToFactoryTable ( ResultElement, ResultSection );

ResultSection::ResultSection ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElementCollection ( shortdesc, longdesc, unit )
{}

ResultSection::ResultSection ( const std::string& sectionName, const std::string& introduction )
    : ResultElementCollection ( "", "", "" ),
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




std::string
ResultSection::latexRepresentation(
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
    std::ostringstream f;
    f << latex_subsection ( documentHierarchyLevel )
      << "{" << SimpleLatex(sectionName_).toLaTeX() << "}\n";
//   f << "\\label{" << cleanSymbols(name) << "}" << std::endl;  // problem with underscores: "\_" as returned by cleanSymbols is wrong here
    f << SimpleLatex(introduction_).toLaTeX() << std::endl;

    f<<ResultElementCollection::latexRepresentation( name, documentHierarchyLevel, fsi );

    return f.str();
}



void ResultSection::insertLatexHeaderCode ( std::set<std::string>& hc ) const
{
    for ( auto& i:
            static_cast<const ResultElement&>(*this) )
    {
        i.insertLatexHeaderCode ( hc );
    }
}

void ResultSection::exportDataToFile (
    const string& name,
    const boost::filesystem::path& outputdirectory ) const
{
    boost::filesystem::path subdir=outputdirectory/name;

    if ( !boost::filesystem::exists ( subdir ) ) {
        boost::filesystem::create_directories ( subdir );
    }

    for ( auto& e:
            static_cast<const ResultElement&>(*this)) {
        if (auto* re=dynamic_cast<const ResultElement*>(&e))
            re->exportDataToFile ( re->name(), subdir );
    }
}

const xml_node< char >*
ResultSection::readFromNode(
    const string &name,
    const rapidxml::xml_node<> &node)
{
  auto *child=ResultElementCollection::readFromNode(name, node);

  sectionName_=getMandatoryAttribute(*child, "sectionName");
  introduction_=getMandatoryAttribute(*child, "introduction");

  return child;
}





xml_node< char >* ResultSection::appendToNode (
    const string& name,
    xml_document< char >& doc,
    xml_node< char >& node,
    const OutputProperties& outProps ) const
{
    auto child = ResultElementCollection::appendToNode ( name, doc, node, outProps );

    appendAttribute(doc, *child, "sectionName", sectionName_);
    appendAttribute(doc, *child, "introduction",  introduction_);

    return child;
}


std::unique_ptr<hierarchicalData::Element> ResultSection::cloneUninitialized() const
{
    auto res = std::make_unique<ResultSection>( sectionName_ );
    for ( auto& re: static_cast<const ResultElement&>(*this) ) {
        res->insert(re.name(), re.cloneAs<ResultElement>() );
    }
    res->setOrder ( order() );
    return res;
}



} // namespace insight
