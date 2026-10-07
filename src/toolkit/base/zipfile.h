#ifndef INSIGHT_ZIPFILE_H
#define INSIGHT_ZIPFILE_H

#include <boost/filesystem.hpp>

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

    /**
     * @brief uncompressTo
     * extract all files into the given directory, streaming them to disk.
     * Parent directories are created as needed.
     * @param targetDirectory
     */
    void uncompressTo(const boost::filesystem::path& targetDirectory) const;
};


/**
 * @brief writeZipFile
 * create a ZIP archive from local files
 * @param zipFilePath
 * the archive to create
 * @param entries
 * map of path inside archive => path of source file on disk
 */
void writeZipFile(
    const boost::filesystem::path& zipFilePath,
    const std::map<boost::filesystem::path, boost::filesystem::path>& entries );

} // namespace insight

#endif // INSIGHT_ZIPFILE_H
