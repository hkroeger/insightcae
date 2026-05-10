#include "image.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;

namespace insight {


defineType ( Image );
addToFactoryTable ( ResultElement, Image );




Image::Image (
    const std::string& shortdesc,
    const std::string& longdesc,
    const std::string& unit )
  : FileResult ( shortdesc, longdesc, unit )
{
    setDisplayFullPage(true);
}




Image::Image
(
    const boost::filesystem::path& location,
    const boost::filesystem::path& value,
    const std::string& shortDesc,
    const std::string& longDesc
)
    : FileResult ( location, value, shortDesc, longDesc )
{
    setDisplayFullPage(true);
}

Image::Image(
    const FileContainer &fc,
    const std::string &shortDesc,
    const std::string &longDesc )
    : FileResult(fc, shortDesc, longDesc)
{
    setDisplayFullPage(true);
}




void Image::insertLatexHeaderCode ( std::set<std::string>& h ) const
{
    h.insert("\\usepackage{graphicx}");
    h.insert("\\usepackage{placeins}");
}




std::string Image::latexRepresentation (
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
  copyTo(fsi.additionalFiles->directory/fileName(), true);

  std::ostringstream f;
  f<<
      "\n\nSee figure below.\n"
     "\\begin{figure}[!h]"
     "\\PlotFrame{keepaspectratio,width=\\textwidth}{"
      << ( fsi.additionalFiles->directoryRelativePath/fileName() ).generic_string() << "}\n"
     "\\caption{"+shortDescription_.toLaTeX()+"}\n"
     "\\end{figure}"
     "\\FloatBarrier"
    ;
  return f.str();
}

int Image::nChildren() const
{
    return 0;
}




std::unique_ptr<hierarchicalData::Element> Image::cloneUninitialized() const
{
    auto res =
        std::make_unique<Image>
        (
          *this,
          shortDescription_.simpleLatex(),
          longDescription_.simpleLatex()
        );
    res->setOrder ( order() );
    return res;
}




} // namespace insight
