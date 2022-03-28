#ifndef INSIGHT_ZIPFILE_H
#define INSIGHT_ZIPFILE_H

#include "base/boost_include.h"

namespace insight {

struct ZipFileImpl;

class ZipFile
{
    std::shared_ptr<ZipFileImpl> zipfile_;

public:
    ZipFile(const boost::filesystem::path& zipFilePath);

    std::set<boost::filesystem::path> files(bool includeDirectories=false) const;

    std::map<boost::filesystem::path, std::shared_ptr<std::string> >
    uncompressFiles(
            const std::set<boost::filesystem::path>& filesToInclude = std::set<boost::filesystem::path>() ) const;
};

} // namespace insight

#endif // INSIGHT_ZIPFILE_H
