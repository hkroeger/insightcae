#ifndef VIDEO_H
#define VIDEO_H

#include "base/resultelements/fileresult.h"

namespace insight {


class Video
    : public FileResult
{

public:
    declareType ( "Video" );

    using FileResult::FileResult;

    void insertLatexHeaderCode ( std::set<std::string>& f ) const override;

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;


    std::unique_ptr<hierarchicalData::Element> clone() const override;
};



} // namespace insight

#endif // VIDEO_H
