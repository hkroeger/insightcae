#include "fileresult.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;


namespace insight {


defineType ( FileResult );
addToFactoryTable ( ResultElement, FileResult );




FileResult::FileResult
(
    const std::string& shortdesc,
    const std::string& longdesc,
    const std::string& unit
)
    : ResultElement ( shortdesc, longdesc, unit )
{}




FileResult::FileResult
(
    const boost::filesystem::path& location,
    const boost::filesystem::path& value,
    const std::string& shortDesc,
    const std::string& longDesc
)
    : ResultElement ( shortDesc, longDesc, "" ),
      FileContainer ( value, location )
{
  if (!hasFileContent())
  {
      replaceContent(location/value);
  }
}

FileResult::FileResult(
    const FileContainer &fc,
    const std::string &shortDesc,
    const std::string &longDesc )
: ResultElement ( shortDesc, longDesc, "" ),
  FileContainer ( fc )
{}




std::string FileResult::latexRepresentation (
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
  std::ostringstream f;
  f<<  "\\texttt{" <<
      SimpleLatex(
        boost::filesystem::absolute(
            filePath()).generic_string() )
        .toLaTeX() << "}";
  return f.str();
}






const rapidxml::xml_node< char >*
FileResult::readFromNode(
    const string &name,
    const rapidxml::xml_node<> &node )
{
    auto *child=ResultElement::readFromNode(name, node);
    FileContainer::readFromNode( *child, "originalFileName", "data");
    return child;
}



int FileResult::nChildren() const
{
    return 0;
}

bool FileResult::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const FileResult*>(&op))
    {
        return FileContainer::operator==(*oa);
    }
    else
        return false;
}



xml_node< char >* FileResult::appendToNode (
    const string& name,
    xml_document< char >& doc,
    xml_node< char >& node,
    const OutputProperties& outProps) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node, outProps );
    FileContainer::appendToNode(doc, *child, "originalFileName", "data");
    return child;
}



std::unique_ptr<hierarchicalData::Element> FileResult::clone() const
{
    auto res =
        std::make_unique<FileResult>
        (
          *this,
          shortDescription_.simpleLatex(),
          longDescription_.simpleLatex()
        );
    res->setOrder ( order() );
    return res;
}


} // namespace insight
