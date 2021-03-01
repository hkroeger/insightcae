#include "fileresult.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
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
    const std::string& longDesc,
    std::shared_ptr<std::string> base64_content
)
    : ResultElement ( shortDesc, longDesc, "" ),
      FileContainer ( absolute ( value, location ), base64_content )
{
  if (!file_content_)
  {
//    pack();
    replaceContent(originalFilePath());
  }
}

void FileResult::writeLatexHeaderCode ( std::ostream& ) const
{}

void FileResult::writeLatexCode ( std::ostream& f, const std::string& , int , const boost::filesystem::path& /*outputfilepath*/ ) const
{
    f<<  "\\texttt{"
       + SimpleLatex( boost::lexical_cast<std::string>(boost::filesystem::absolute(originalFilePath_)) ).toLaTeX()
         + "}";
}




path FileResult::filePath(path baseDirectory) const
{
  auto up=unpackFilePath(baseDirectory);

  if (needsUnpack(up))
  {
    copyTo(up, true);
  }

  return up;
}




void FileResult::readFromNode(const string &name, rapidxml::xml_document<> &doc, rapidxml::xml_node<> &node)
{
 readBaseAttributesFromNode(name, doc, node);
 FileContainer::readFromNode(doc, node, ".", "originalFileName", "data");
}

xml_node< char >* FileResult::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );
    FileContainer::appendToNode(doc, *child, ".", "originalFileName", "data");
    return child;
}


ResultElementPtr FileResult::clone() const
{
    ResultElementPtr res =
        std::make_shared<FileResult>
        (
          originalFilePath_.parent_path(),
          originalFilePath_,
          shortDescription_.simpleLatex(),
          longDescription_.simpleLatex(),
          file_content_
        );
    res->setOrder ( order() );
    return res;
}


} // namespace insight
