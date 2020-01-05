#include "image.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;

namespace insight {


defineType ( Image );
addToFactoryTable ( ResultElement, Image );


Image::Image ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{
}

Image::Image ( const boost::filesystem::path& location, const boost::filesystem::path& value, const std::string& shortDesc, const std::string& longDesc )
    : ResultElement ( shortDesc, longDesc, "" ),
      imagePath_ ( absolute ( value, location ) )
{
}

void Image::writeLatexHeaderCode ( std::ostream& f ) const
{
    f<<"\\usepackage{graphicx}\n";
    f<<"\\usepackage{placeins}\n";
}

void Image::writeLatexCode ( std::ostream& f, const std::string& , int , const boost::filesystem::path& outputfilepath ) const
{
    //f<< "\\includegraphics[keepaspectratio,width=\\textwidth]{" << cleanSymbols(imagePath_.c_str()) << "}\n";
    f<<
     "\n\nSee figure below.\n"
     "\\begin{figure}[!h]"
     "\\PlotFrame{keepaspectratio,width=\\textwidth}{" << make_relative ( outputfilepath, imagePath_ ).c_str() << "}\n"
     "\\caption{"+shortDescription_.toLaTeX()+"}\n"
     "\\end{figure}"
                                              "\\FloatBarrier";
}

void Image::readFromNode(const string &name, rapidxml::xml_document<> &doc, rapidxml::xml_node<> &node)
{
 readBaseAttributesFromNode(name, doc, node);
 boost::filesystem::path originalFileName (node.first_attribute("originalFileName")->value());
 imagePath_ = boost::filesystem::unique_path(
       boost::filesystem::temp_directory_path() /
        (originalFileName.filename().stem().string() + "-%%%%%%%%" + originalFileName.extension().string())
       );
 std::string contents = base64_decode(node.first_attribute("data")->value());
 std::ofstream f(imagePath_.c_str());
 f << contents;
}

xml_node< char >* Image::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    child->append_attribute(
          doc.allocate_attribute(
            "originalFileName",
            doc.allocate_string( imagePath_.c_str() )
            )
          );

    child->append_attribute(
          doc.allocate_attribute(
            "data",
            doc.allocate_string(base64_encode ( imagePath_ ).c_str())
            )
          );
//    child->value
//    (
//        doc.allocate_string
//        (
//            base64_encode ( imagePath_ ).c_str()
//        )
//    );

    return child;
}


ResultElementPtr Image::clone() const
{
    ResultElementPtr res ( new Image ( imagePath_.parent_path(), imagePath_, shortDescription_.simpleLatex(), longDescription_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}


} // namespace insight
