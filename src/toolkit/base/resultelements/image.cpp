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
    : FileResult ( shortdesc, longdesc, unit )
{}




Image::Image
(
    const boost::filesystem::path& location,
    const boost::filesystem::path& value,
    const std::string& shortDesc,
    const std::string& longDesc,
    std::shared_ptr<std::string> base64_content
)
    : FileResult ( location, value, shortDesc, longDesc, base64_content )
{}




void Image::writeLatexHeaderCode ( std::ostream& f ) const
{
    f<<"\\usepackage{graphicx}\n";
    f<<"\\usepackage{placeins}\n";
}




void Image::writeLatexCode (
    std::ostream& f,
    const std::string& ,
    int ,
    const boost::filesystem::path& outputfilepath ) const
{
  auto up=unpackFilePath(outputfilepath);
  if (needsUnpack(up))
    copyTo(up);

  //f<< "\\includegraphics[keepaspectratio,width=\\textwidth]{" << cleanSymbols(imagePath_.c_str()) << "}\n";
  f<<
      "\n\nSee figure below.\n"
     "\\begin{figure}[!h]"
     "\\PlotFrame{keepaspectratio,width=\\textwidth}{"
      << make_relative ( outputfilepath, up ).c_str() << "}\n"
     "\\caption{"+shortDescription_.toLaTeX()+"}\n"
     "\\end{figure}"
     "\\FloatBarrier"
    ;
}




ResultElementPtr Image::clone() const
{
    ResultElementPtr res =
        std::make_shared<Image>
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
