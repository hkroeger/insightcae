#ifndef INSIGHT_IMAGE_H
#define INSIGHT_IMAGE_H

#include "base/resultelements/fileresult.h"

namespace insight {


class Image
    : public FileResult
{

public:
    declareType ( "Image" );

    Image
        (
            const std::string& shortdesc,
            const std::string& longdesc,
            const std::string& unit
            );

    Image
        (
            const boost::filesystem::path& location,
            const boost::filesystem::path& value,
            const std::string& shortDesc,
            const std::string& longDesc
            );

    Image
        (
            const FileContainer& fc,
            const std::string& shortDesc,
            const std::string& longDesc
            );


    void insertLatexHeaderCode ( std::set<std::string>& f ) const override;
    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    int nChildren() const override;
protected:
    std::unique_ptr<hierarchicalData::Element> cloneUninitialized() const override;
};



} // namespace insight

#endif // INSIGHT_IMAGE_H
