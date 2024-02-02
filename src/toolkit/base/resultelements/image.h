#ifndef INSIGHT_IMAGE_H
#define INSIGHT_IMAGE_H

#include "base/resultelements/fileresult.h"

namespace insight {


class Image
    : public FileResult
{

public:
    declareType ( "Image" );

    Image ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    Image
    (
        const boost::filesystem::path& location,
        const boost::filesystem::path& value,
        const std::string& shortDesc,
        const std::string& longDesc,
        std::shared_ptr<std::string> base64_content = std::shared_ptr<std::string>()
    );


    void insertLatexHeaderCode ( std::set<std::string>& f ) const override;
    void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const override;


    ResultElementPtr clone() const override;
};



} // namespace insight

#endif // INSIGHT_IMAGE_H
