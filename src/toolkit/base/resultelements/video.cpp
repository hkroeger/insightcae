#include "video.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;

namespace insight {


defineType ( Video );
addToFactoryTable ( ResultElement, Video );




Video::Video ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : FileResult ( shortdesc, longdesc, unit )
{}




Video::Video
    (
        const boost::filesystem::path& location,
        const boost::filesystem::path& value,
        const std::string& shortDesc,
        const std::string& longDesc,
        std::shared_ptr<std::string> base64_content
        )
    : FileResult ( location, value, shortDesc, longDesc, base64_content )
{}




void Video::insertLatexHeaderCode ( std::set<std::string>& h ) const
{
}




void Video::writeLatexCode (
    std::ostream& f,
    const std::string& ,
    int ,
    const boost::filesystem::path& outputfilepath ) const
{
    // auto up=unpackFilePath(outputfilepath);
    // if (needsUnpack(up))
    //     copyTo(up, true);

    // f<<
    //     "\n\nSee figure below.\n"
    //     "\\begin{figure}[!h]"
    //     "\\PlotFrame{keepaspectratio,width=\\textwidth}{"
    //   << make_relative ( outputfilepath, up ).generic_path().string() << "}\n"
    //                                                                   "\\caption{"+shortDescription_.toLaTeX()+"}\n"
    //                                                                                                       "\\end{figure}"
    //                                                                                                       "\\FloatBarrier"
    //     ;
}




ResultElementPtr Video::clone() const
{
    ResultElementPtr res =
        std::make_shared<Video>
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
