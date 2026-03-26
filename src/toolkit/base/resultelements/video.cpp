#include "video.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;

namespace insight {


defineType ( Video );
addToFactoryTable ( ResultElement, Video );






void Video::insertLatexHeaderCode ( std::set<std::string>& h ) const
{
}




std::string Video::latexRepresentation(
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
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
    return "";
}




std::unique_ptr<hierarchicalData::Element> Video::clone() const
{
    auto res =
        std::make_unique<Video>
        (
            *this,
            shortDescription_.simpleLatex(),
            longDescription_.simpleLatex()
            );
    res->setOrder ( order() );
    return res;
}




} // namespace insight
